// CompressEngine.cpp

#include "StdAfx.h"

#include "CompressEngine.h"

#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/Thread.h"
#include "Windows/COM.h"

#include "Common/StringConvert.h"

#include "CompressDialog.h"

#include "../Common/CompressEngineCommon.h"
#include "../Common/UpdateUtils.h"
#include "../Common/FolderArchiveInterface.h"

#include "../Common/OpenEngine2.h"
#include "../../FileManager/ProgramLocation.h"

#include "../Agent/Handler.h"

#include "../Resource/Extract/resource.h"

#include "MyMessages.h"
#include "../../FileManager/FormatUtils.h"
#include "../../FileManager/UpdateCallback100.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;
using namespace NName;

using namespace NUpdateArchive;

static LPCTSTR kTempArcivePrefix = TEXT("7zi");
static LPCTSTR kDefaultSfxModule = TEXT("7zC.sfx");


/*
// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
*/

static void SplitString(const CSysString &srcString, CSysStringVector &destStrings)
{
  destStrings.Clear();
  for (int pos = 0; pos < srcString.Length();)
  {
    int spacePos = srcString.Find(TEXT(' '), pos);
    if (spacePos < 0)
    {
      destStrings.Add(srcString.Mid(pos));
      return;
    }
    if (spacePos != pos)
      destStrings.Add(srcString.Mid(pos, spacePos - pos));
    pos = spacePos + 1;
  }
}

static bool ParseNumberString(const UString &string, UINT32 &number)
{
  wchar_t *endPtr;
  number = wcstoul(string, &endPtr, 10);
  return (endPtr - string == string.Length());
}

static void SetOptions(const CSysString &options,
    CObjectVector<CComBSTR> &realNames,
    std::vector<NCOM::CPropVariant> &values)
{
  CSysStringVector strings;
  SplitString(options, strings);
  for(int i = 0; i < strings.Size(); i++)
  {
    const UString &string = GetUnicodeString(strings[i]);
    int index = string.Find(L'=');
    CComBSTR name;
    NCOM::CPropVariant propVariant;
    if (index < 0)
      name = string;
    else
    {
      name = string.Left(index);
      UString value = string.Mid(index + 1);
      if (!value.IsEmpty())
      {
        UINT32 number;
        if (ParseNumberString(value, number))
          propVariant = number;
        else
          propVariant = value;
      }
    }
    realNames.Add(name);
    values.push_back(propVariant);
  }
}

static HRESULT SetOutProperties(IOutFolderArchive * outArchive, 
    UINT32 method, bool solidModeIsAllowed, bool solidMode, 
    bool sfxMode,
    const CSysString &options)
{
  CComPtr<ISetProperties> setProperties;
  if (outArchive->QueryInterface(&setProperties) == S_OK)
  {
    CComBSTR comBSTR;
    switch(method)
    {
      case 0:
        comBSTR = "0";
        break;
      case 1:
        comBSTR = "1";
        break;
      case 2:
        comBSTR = "X";
        break;
      default:
        return E_INVALIDARG;
    }
    CObjectVector<CComBSTR> realNames;
    std::vector<NCOM::CPropVariant> values;
    realNames.Add(comBSTR);
    values.push_back(NCOM::CPropVariant());

    if (sfxMode)
    {
      realNames.Add(L"rsfx");
      values.push_back(NCOM::CPropVariant(L"on"));
    }
   
    // Solid
    if (solidModeIsAllowed)
    {
      realNames.Add(L"s");
      values.push_back(NCOM::CPropVariant(solidMode ? L"on": L"off"));
    }

    // Options 
    SetOptions(options, realNames, values);
    
    std::vector<BSTR> names;
    for(int i = 0; i < realNames.Size(); i++)
      names.push_back(realNames[i]);
    RETURN_IF_NOT_S_OK(setProperties->SetProperties(&names.front(), 
      &values.front(), names.size()));
  }
  return S_OK;
}

struct CThreadUpdateCompress
{
  CComPtr<IOutFolderArchive> OutArchive;
  CLSID ClassID;
  UString OutArchivePath;
  BYTE ActionSetByte[NUpdateArchive::NPairState::kNumValues];
  bool SfxMode;
  UString SfxModule;
  

  UStringVector FileNames;
  CRecordVector<const wchar_t *> FileNamePointers;
  CComPtr<IFolderArchiveUpdateCallback> UpdateCallback;
  CComObjectNoLock<CUpdateCallback100Imp> *UpdateCallbackSpec;
  HRESULT Result;
  
  DWORD Process()
  {
    NCOM::CComInitializer comInitializer;
    UpdateCallbackSpec->ProgressDialog.WaitCreating();
    Result = OutArchive->DoOperation(&ClassID,
      OutArchivePath, ActionSetByte, 
      (SfxMode ? (const wchar_t *)SfxModule: NULL),
      UpdateCallback);
    UpdateCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }

  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadUpdateCompress *)param)->Process();
  }
};

HRESULT CompressArchive(const CSysStringVector &fileNames)
{
  if (fileNames.Size() == 0)
    return S_OK;

  UString currentDirU;
  const CSysString &frontName = fileNames.Front();
  UString frontNameU = GetUnicodeString(frontName);
  /*
  if (!GetOnlyDirPrefix(frontName, aCurrentDir))
    return E_FAIL;
  */

  UString resultPathU;
  {
    /*
    bool aPrevFolderOK = false;
    if (aCurrentDir.Length() > 3)
      aPrevFolderOK = GetOnlyName(aCurrentDir.Left(aCurrentDir.Length() - 1), aResultPath);
    if (fileNames.Size() == 1 || !aPrevFolderOK)
    {
      if (!GetOnlyName(frontName, aResultPath))
        return E_FAIL;
    }
    */
    CParsedPath parsedPath;
    parsedPath.ParsePath(frontNameU);
    if(parsedPath.PathParts.Size() == 0)
      return E_FAIL; // Error
    if (fileNames.Size() == 1 || parsedPath.PathParts.Size() == 1)
    {
      CSysString pureName, dot, extension;
      resultPathU = parsedPath.PathParts.Back();
    }
    else
    {
      parsedPath.PathParts.DeleteBack();
      resultPathU = parsedPath.PathParts.Back();
    }
  }
  
  {
    CParsedPath parsedPath;
    parsedPath.ParsePath(frontNameU);
    parsedPath.PathParts.DeleteBack();
    currentDirU = parsedPath.MergePath();
    if (parsedPath.PathParts.Size() > 0)
      currentDirU += NFile::NName::kDirDelimiter;
  }

  CCompressDialog dialog;

  CObjectVector<NZipRootRegistry::CArchiverInfo> archiverInfoList;
  NZipRootRegistry::ReadArchiverInfoList(archiverInfoList);
  dialog.m_ArchiverInfoList.Clear();
  for(int i = 0; i < archiverInfoList.Size(); i++)
  {
    const NZipRootRegistry::CArchiverInfo &archiverInfo = archiverInfoList[i];
    if (archiverInfo.UpdateEnabled)
      dialog.m_ArchiverInfoList.Add(archiverInfo);
  }
  if(dialog.m_ArchiverInfoList.Size() == 0)
  {
    MyMessageBox(L"No Update Engines");
    return E_FAIL;
  }

  dialog.m_Info.ArchiveName = GetSystemString(resultPathU);
  dialog.m_Info.CurrentDirPrefix = GetSystemString(currentDirU);
  dialog.m_Info.SFXMode = false;
  dialog.m_Info.SolidMode = true;

  if(dialog.Create(0) != IDOK)
    return S_OK;

  CSysString arcPath;
  if (!dialog.m_Info.GetFullPathName(arcPath))
  {
    MyMessageBox(L"Incorrect archive path");
    return E_FAIL;
  }
  const CActionSet *actionSet;
  switch(dialog.m_Info.UpdateMode)
  {
    case NCompressDialog::NUpdateMode::kAdd:
      actionSet = &kAddActionSet;
      break;
    case NCompressDialog::NUpdateMode::kUpdate:
      actionSet = &kUpdateActionSet;
      break;
    case NCompressDialog::NUpdateMode::kFresh:
      actionSet = &kFreshActionSet;
      break;
    case NCompressDialog::NUpdateMode::kSynchronize:
      actionSet = &kSynchronizeActionSet;
      break;
    default:
      throw 1091756;
  }

  NZipSettings::NWorkDir::CInfo workDirInfo;
  NZipRegistryManager::ReadWorkDirInfo(workDirInfo);
  CSysString aWorkDir = GetWorkDir(workDirInfo, arcPath);
  NFile::NDirectory::CreateComplexDirectory(aWorkDir);

  NFile::NDirectory::CTempFile tempFile;
  CSysString tempFileName;
  if (tempFile.Create(aWorkDir, kTempArcivePrefix, tempFileName) == 0)
    return E_FAIL;

  const CLSID &classID = 
      dialog.m_ArchiverInfoList[dialog.m_Info.ArchiverInfoIndex].ClassID;
  
  NFind::CFileInfo fileInfo;

  CComPtr<IOutFolderArchive> outArchive;

  CComPtr<IInFolderArchive> archiveHandler;
  if(NFind::FindFile(arcPath, fileInfo))
  {
    if (fileInfo.IsDirectory())
    {
      MyMessageBox(L"There is a folder with such name");
      return E_FAIL;
    }
    NZipRootRegistry::CArchiverInfo archiverInfoResult;
    UString defaultName;
    HRESULT result = OpenArchive(arcPath, &archiveHandler, 
        archiverInfoResult, defaultName, NULL);
    if (result == S_FALSE)
    {
      MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      return E_FAIL;
    }
    if (result != S_OK)
    {
      MyMessageBox(L"Open error");
      return E_FAIL;
    }
    if (archiverInfoResult.ClassID != classID)
    {
      MyMessageBox(L"Type of existing archive differs from specified type");
      return E_FAIL;
    }
    result = archiveHandler.QueryInterface(&outArchive);
    if(result != S_OK)
    {
      MyMessageBox(MyFormatNew(IDS_CANT_UPDATE_ARCHIVE, 0x02000602, 
          GetUnicodeString(arcPath)));
      return E_FAIL;
    }
  }
  else
  {
    // HRESULT result = outArchive.CoCreateInstance(classID);
    CComObjectNoLock<CAgent> *agentSpec = new CComObjectNoLock<CAgent>;
    outArchive = agentSpec;

    /*
    HRESULT result = outArchive.CoCreateInstance(CLSID_CAgentArchiveHandler);
    if (result != S_OK)
    {
      MyMessageBox(MyFormat(IDS_CANT_UPDATE_ARCHIVE, arcPath));
      return E_FAIL;
    }
    */
  }

  CRecordVector<const wchar_t *> fileNamePointers;
  fileNamePointers.Reserve(fileNames.Size());

  UStringVector fileNames2;
  for(i = 0; i < fileNames.Size(); i++)
    fileNames2.Add(GetUnicodeString(fileNames[i]));

  for(i = 0; i < fileNames.Size(); i++)
    fileNamePointers.Add(fileNames2[i]);

  outArchive->SetFolder(NULL);

  // Don't uses CurrentFolder here, since files are absolute paths;
  // MyGetCurrentDirectory(aCurrentFolder);

  UINT codePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  outArchive->SetFiles(L"", 
      &fileNamePointers.Front(), fileNamePointers.Size());

  CThreadUpdateCompress updater;
  for (i = 0; i < NUpdateArchive::NPairState::kNumValues; i++)
    updater.ActionSetByte[i] = actionSet->StateActions[i];
  updater.UpdateCallbackSpec = new CComObjectNoLock<CUpdateCallback100Imp>;
  updater.UpdateCallback = updater.UpdateCallbackSpec;
  updater.OutArchive = outArchive;
  // updater.SrcFolderPrefix = srcPanel._currentFolderPrefix;

  CSysString title = LangLoadString(IDS_PROGRESS_COMPRESSING, 0x02000DC0);
  updater.UpdateCallbackSpec->Init(0, !dialog.Password.IsEmpty(), 
      GetUnicodeString(dialog.Password));

  RETURN_IF_NOT_S_OK(SetOutProperties(outArchive, dialog.m_Info.Method, 
      dialog.m_Info.SolidModeIsAllowed, dialog.m_Info.SolidMode, 
      dialog.m_Info.SFXMode,
      dialog.m_Info.Options));

  UString sfxModule;
  if (dialog.m_Info.SFXMode)
  {
    CSysString sfxModule2;
    LPCTSTR path = NULL;
    CSysString sfxModule3;
    if (GetProgramFolderPath(sfxModule3))
      path = sfxModule3;
    if (!NDirectory::MySearchPath(path, kDefaultSfxModule, NULL, sfxModule2))
    {
      MyMessageBox(L"can't find sfx module");
      return E_FAIL;
    }
    sfxModule = GetUnicodeString(sfxModule2);
  }

  updater.OutArchivePath = GetUnicodeString(tempFileName, codePage);
  updater.SfxMode = dialog.m_Info.SFXMode;
  updater.SfxModule = sfxModule;
  updater.ClassID = classID;

  CThread thread;
  if (!thread.Create(CThreadUpdateCompress::MyThreadFunction, &updater))
    throw 271824;
  updater.UpdateCallbackSpec->StartProgressDialog(title);
  HRESULT result = updater.Result;

  updater.UpdateCallback.Release();
  
  updater.OutArchive.Release();
  outArchive.Release();

  if (result != S_OK)
  {
    ShowErrorMessage(result);
    return result;
  }
 
  if(archiveHandler)
  {
    archiveHandler->Close();
    if (!DeleteFileAlways(arcPath))
    {
      ShowLastErrorMessage();
      return E_FAIL;
    }
  }
  tempFile.DisableDeleting();
  if (!::MoveFile(tempFileName, arcPath))
  {
    ShowLastErrorMessage();
    return E_FAIL;
  }
  return S_OK;
}
