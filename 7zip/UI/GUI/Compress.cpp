// Compress.cpp

#include "StdAfx.h"

#include <mapi.h>

#include "Compress.h"
#include "CompressDialog.h"
#include "resource.h"

#include "Common/StringConvert.h"
#include "Windows/FileName.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/Thread.h"
#include "Windows/COM.h"
#include "Windows/PropVariant.h"

#include "../../FileManager/ProgramLocation.h"
#include "../../FileManager/FormatUtils.h"
#include "../../FileManager/UpdateCallback100.h"

#include "../Agent/Agent.h"
#include "../Common/UpdateAction.h"
#include "../Common/WorkDir.h"
#include "../Common/ZipRegistry.h"
#include "../Common/OpenArchive.h"
#include "../Common/ArchiveName.h"
#include "../Resource/Extract/resource.h"
#include "../Explorer/MyMessages.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;
using namespace NName;

static LPCTSTR kTempArchivePrefix = TEXT("7zA");
static LPCTSTR kTempFolderPrefix = TEXT("7zE");
static LPCTSTR kDefaultSfxModule = TEXT("7zC.sfx");

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
    CObjectVector<CMyComBSTR> &realNames,
    std::vector<NCOM::CPropVariant> &values)
{
  CSysStringVector strings;
  SplitString(options, strings);
  for(int i = 0; i < strings.Size(); i++)
  {
    const UString &string = GetUnicodeString(strings[i]);
    int index = string.Find(L'=');
    CMyComBSTR name;
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
    UINT32 method, 
    bool solidModeIsAllowed, bool solidMode, 
    bool multiThreadIsAllowed, bool multiThread, 
    bool encryptHeadersIsAllowed, bool encryptHeaders,
    bool sfxMode,
    const CSysString &options)
{
  CMyComPtr<ISetProperties> setProperties;
  if (outArchive->QueryInterface(&setProperties) == S_OK)
  {
    CMyComBSTR comBSTR = L"x";
    CObjectVector<CMyComBSTR> realNames;
    std::vector<NCOM::CPropVariant> values;
    realNames.Add(comBSTR);
    values.push_back(NCOM::CPropVariant((UINT32)method));

    if (sfxMode)
    {
      realNames.Add(L"rsfx");
      values.push_back(NCOM::CPropVariant(L"on"));
    }

    if (encryptHeadersIsAllowed)
    {
      if (encryptHeaders)
      {
        realNames.Add(L"he");
        values.push_back(NCOM::CPropVariant(L"on"));
      }
    }
   
    // Solid
    if (solidModeIsAllowed)
    {
      realNames.Add(L"s");
      values.push_back(NCOM::CPropVariant(solidMode ? L"on": L"off"));
    }
    if (multiThreadIsAllowed)
    {
      realNames.Add(L"mt");
      values.push_back(NCOM::CPropVariant(multiThread ? L"on": L"off"));
    }

    // Options 
    SetOptions(options, realNames, values);
    
    std::vector<BSTR> names;
    for(int i = 0; i < realNames.Size(); i++)
      names.push_back(realNames[i]);
    RINOK(setProperties->SetProperties(&names.front(), 
      &values.front(), names.size()));
  }
  return S_OK;
}

struct CThreadUpdateCompress
{
  CMyComPtr<IOutFolderArchive> OutArchive;
  UString LibPath;
  CLSID ClassID;
  UString OutArchivePath;
  BYTE ActionSetByte[NUpdateArchive::NPairState::kNumValues];
  bool SfxMode;
  UString SfxModule;
  

  UStringVector FileNames;
  CRecordVector<const wchar_t *> FileNamePointers;
  CMyComPtr<IFolderArchiveUpdateCallback> UpdateCallback;
  CUpdateCallback100Imp *UpdateCallbackSpec;
  HRESULT Result;
  
  DWORD Process()
  {
    NCOM::CComInitializer comInitializer;
    UpdateCallbackSpec->ProgressDialog.WaitCreating();
    Result = OutArchive->DoOperation(
      LibPath, &ClassID,
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

static CSysString MakeFullArchiveName(const CSysString &name, 
    const UString &extension)
{
  if (extension.IsEmpty())
    return name;
  if (name.IsEmpty())
    return name;
  if (name[name.Length() - 1] == '.')
    return name.Left(name.Length() - 1);
  int slash1Pos = name.ReverseFind(L'\\');
  int slash2Pos = name.ReverseFind(L'/');
  int slashPos = MyMax(slash1Pos, slash2Pos);
  int dotPos = name.ReverseFind(L'.');
  if (dotPos >= 0 && (dotPos > slashPos || slashPos < 0))
    return name;
  return name + L'.' + GetSystemString(extension);
}

HRESULT CompressArchive(const CSysStringVector &fileNames, 
    const CSysString &archiveName, bool email)
{
  if (fileNames.Size() == 0)
    return S_OK;

  CObjectVector<CArchiverInfo> archivers;
  ReadArchiverInfoList(archivers);

  CArchiverInfo archiverInfo;
  UString password;
  bool encryptHeadersIsAllowed = false;
  bool encryptHeaders = false;
  const NUpdateArchive::CActionSet *actionSet;
  NCompressDialog::CInfo compressInfo;
  bool keepName = false;

  const CSysString &frontName = fileNames.Front();
  CSysString tempDirPath;
  CSysString currentDirPrefix;
  bool needTempFile = true;
  NDirectory::CTempDirectory tempDirectory;
  if (email)
  {
    tempDirectory.Create(kTempFolderPrefix);
    currentDirPrefix = tempDirectory.GetPath();
    NormalizeDirPathPrefix(currentDirPrefix);
    // MyMessageBox(currentDirPrefix);
    needTempFile = false;
  }
  else
    NDirectory::GetOnlyDirPrefix(frontName, currentDirPrefix);

  if (archiveName.IsEmpty())
  {
    CCompressDialog dialog;
    for(int i = 0; i < archivers.Size(); i++)
    {
      const CArchiverInfo &archiverInfo = archivers[i];
      if (archiverInfo.UpdateEnabled)
        dialog.m_ArchiverInfoList.Add(archiverInfo);
    }
    if(dialog.m_ArchiverInfoList.Size() == 0)
    {
      MyMessageBox(L"No Update Engines");
      return E_FAIL;
    }
    dialog.m_Info.ArchiveName = CreateArchiveName(frontName, 
        fileNames.Size() > 1, 
        // dialog.m_ArchiverInfoList[0].KeepName
        true);

    dialog.m_Info.CurrentDirPrefix = currentDirPrefix;
    dialog.m_Info.SFXMode = false;
    dialog.m_Info.Solid = true;
    dialog.m_Info.MultiThread = false;
    // dialog.m_Info.KeepName = false;
    if (!keepName && fileNames.Size() == 1)
    {
      NFind::CFileInfo fileInfo;
      if (NFind::FindFile(fileNames.Front(), fileInfo))
        keepName = fileInfo.IsDirectory();
    }

    dialog.m_Info.KeepName = keepName;
    
    /*
    if (keepName)
      MessageBeep(-1);
    */

    if(dialog.Create(0) != IDOK)
      return S_OK;
    
    if (dialog.m_Info.VolumeSizeIsDefined)
    {
      MessageBox(0, TEXT("Splitting to volumes is not supported"), 
          TEXT("7-Zip"), 0);
      return E_FAIL;
    }

    switch(dialog.m_Info.UpdateMode)
    {
      case NCompressDialog::NUpdateMode::kAdd:
        actionSet = &NUpdateArchive::kAddActionSet;
        break;
      case NCompressDialog::NUpdateMode::kUpdate:
        actionSet = &NUpdateArchive::kUpdateActionSet;
        break;
      case NCompressDialog::NUpdateMode::kFresh:
        actionSet = &NUpdateArchive::kFreshActionSet;
        break;
      case NCompressDialog::NUpdateMode::kSynchronize:
        actionSet = &NUpdateArchive::kSynchronizeActionSet;
        break;
      default:
        throw 1091756;
    }
    archiverInfo = dialog.m_ArchiverInfoList[dialog.m_Info.ArchiverInfoIndex];
    password = GetUnicodeString(dialog.Password);
    encryptHeadersIsAllowed = dialog.EncryptHeadersIsAllowed;
    encryptHeaders = dialog.EncryptHeaders;
    compressInfo = dialog.m_Info;
    compressInfo.ArchiveName = MakeFullArchiveName(compressInfo.ArchiveName, 
        archiverInfo.GetMainExtension());
  }
  else
  {
    int i;
    for(i = 0; i < archivers.Size(); i++)
    {
      if (archivers[i].Name.CollateNoCase(L"7z") == 0)
      {
        archiverInfo = archivers[i];
        break;
      }
    }
    if (i == archivers.Size())
    {
      MyMessageBox(L"No archiver");
      return E_FAIL;
    }
    actionSet = &NUpdateArchive::kAddActionSet;
    compressInfo.SolidIsAllowed = true;
    compressInfo.Solid = true;
    compressInfo.MultiThreadIsAllowed = true;
    compressInfo.MultiThread = false;
    compressInfo.SFXMode = false;
    compressInfo.KeepName = false;
    // MessageBox(0, archiveName, TEXT(""), 0);
    compressInfo.ArchiveName = archiveName;
    compressInfo.CurrentDirPrefix = currentDirPrefix;
    compressInfo.Method = NCompressDialog::NMethod::kNormal;
  }
  CSysString arcPath;
  if (!compressInfo.GetFullPathName(arcPath))
  {
    MyMessageBox(L"Incorrect archive path");
    return E_FAIL;
  }
  if (compressInfo.ArchiveName.Find('\\') >= 0)
  {
    needTempFile = true;
  }

  // MessageBox(0, arcPath, 0, 0);

  NWorkDir::CInfo workDirInfo;
  ReadWorkDirInfo(workDirInfo);
  CSysString workDir = GetWorkDir(workDirInfo, arcPath);
  NFile::NDirectory::CreateComplexDirectory(workDir);

  NFile::NDirectory::CTempFile tempFile;
  CSysString tempFileName;
  if (needTempFile)
  {
    if (tempFile.Create(workDir, kTempArchivePrefix, tempFileName) == 0)
      return E_FAIL;
  }
  else
     tempFileName = arcPath;


  /*
  const CLSID &classID = 
      dialog.m_ArchiverInfoList[dialog.m_Info.ArchiverInfoIndex].ClassID;
  */
  NFind::CFileInfo fileInfo;

  CMyComPtr<IOutFolderArchive> outArchive;

  CMyComPtr<IInFolderArchive> archiveHandler;
  if(NFind::FindFile(arcPath, fileInfo))
  {
    if (fileInfo.IsDirectory())
    {
      MyMessageBox(L"There is a folder with such name");
      return E_FAIL;
    }
    CAgent *agentSpec = new CAgent;
    archiveHandler = agentSpec;
    // CLSID realClassID;
    CMyComBSTR archiveType;
    HRESULT result = agentSpec->Open(
        GetUnicodeString(arcPath), &archiveType, NULL);
    if (result == S_FALSE)
    {
      MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      return E_FAIL;
    }
    /*
    HRESULT result = OpenArchive(arcPath, &archiveHandler, 
        archiverInfoResult, defaultName, NULL);
    if (result == S_FALSE)
    {
      MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      return E_FAIL;
    }
    */
    if (result != S_OK)
    {
      MyMessageBox(L"Open error");
      return E_FAIL;
    }
    if (archiverInfo.Name.CollateNoCase((const wchar_t *)archiveType) != 0)
    {
      MyMessageBox(L"Type of existing archive differs from specified type");
      return E_FAIL;
    }
    result = archiveHandler.QueryInterface(IID_IOutFolderArchive, &outArchive);
    if(result != S_OK)
    {
      MyMessageBox(MyFormatNew(IDS_CANT_UPDATE_ARCHIVE, 0x02000602, 
          GetUnicodeString(arcPath)));
      return E_FAIL;
    }
  }
  else
  {
    CAgent *agentSpec = new CAgent;
    outArchive = agentSpec;
  }

  CRecordVector<const wchar_t *> fileNamePointers;
  fileNamePointers.Reserve(fileNames.Size());

  UStringVector fileNames2;
  for(int i = 0; i < fileNames.Size(); i++)
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
  updater.UpdateCallbackSpec = new CUpdateCallback100Imp;
  updater.UpdateCallback = updater.UpdateCallbackSpec;
  updater.OutArchive = outArchive;
  // updater.SrcFolderPrefix = srcPanel._currentFolderPrefix;

  CSysString title = LangLoadString(IDS_PROGRESS_COMPRESSING, 0x02000DC0);
  updater.UpdateCallbackSpec->Init(0, !password.IsEmpty(), password);

  UINT32 methodMap[] = { 0, 1, 5, 9 }; 
  UINT32 method = compressInfo.Method >= sizeof(methodMap) / sizeof(methodMap[0]) ? 
      9 : methodMap[compressInfo.Method];
  HRESULT result = SetOutProperties(outArchive, 
      method, 
      compressInfo.SolidIsAllowed, compressInfo.Solid, 
      compressInfo.MultiThreadIsAllowed, compressInfo.MultiThread, 
      encryptHeadersIsAllowed, encryptHeaders,
      compressInfo.SFXMode,
      compressInfo.Options);

  if (result != S_OK)
  {
    if (result != E_ABORT)
      ShowErrorMessage(result);
    return result;
  }


  UString sfxModule;
  if (compressInfo.SFXMode)
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
  updater.SfxMode = compressInfo.SFXMode;
  updater.SfxModule = sfxModule;
  updater.LibPath = GetUnicodeString(archiverInfo.FilePath);
  updater.ClassID = archiverInfo.ClassID;

  CThread thread;
  if (!thread.Create(CThreadUpdateCompress::MyThreadFunction, &updater))
    throw 271824;
  updater.UpdateCallbackSpec->StartProgressDialog(title);
  result = updater.Result;

  updater.UpdateCallback.Release();
  
  updater.OutArchive.Release();
  outArchive.Release();

  if (result != S_OK)
  {
    if (result != E_ABORT)
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
  if (needTempFile)
  {
    tempFile.DisableDeleting();
    if (!::MoveFile(tempFileName, arcPath))
    {
      ShowLastErrorMessage();
      return E_FAIL;
    }
  }

  if (email)
  {
    NDLL::CLibrary mapiLib;
    if (!mapiLib.Load(TEXT("Mapi32.dll")))
      return E_FAIL;
    LPMAPISENDDOCUMENTS fnSend = (LPMAPISENDDOCUMENTS)
        mapiLib.GetProcAddress("MAPISendDocuments");
    if (fnSend == 0)
      return E_FAIL;

    CSysString fileName;
    GetOnlyName(arcPath, fileName);
    AString path = GetAnsiString(arcPath);
    AString name = GetAnsiString(fileName);
    fnSend(0, ";", (LPSTR)(LPCSTR)path, (LPSTR)(LPCSTR)name, 0); 
  }

  return S_OK;
}
