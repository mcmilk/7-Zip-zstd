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
#include "../Resource/Extract/resource.h"
#include "../Explorer/MyMessages.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;
using namespace NName;

static LPCWSTR kTempArchivePrefix = L"7zA";
static LPCWSTR kTempFolderPrefix = L"7zE";
static LPCWSTR kDefaultSfxModule = L"7zC.sfx";

static void SplitString(const UString &srcString, UStringVector &destStrings)
{
  destStrings.Clear();
  for (int pos = 0; pos < srcString.Length();)
  {
    int spacePos = srcString.Find(L' ', pos);
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

static void SetOptions(const UString &options,
    CObjectVector<CMyComBSTR> &realNames,
    std::vector<NCOM::CPropVariant> &values)
{
  UStringVector strings;
  SplitString(options, strings);
  for(int i = 0; i < strings.Size(); i++)
  {
    const UString &s = strings[i];
    int index = s.Find(L'=');
    CMyComBSTR name;
    NCOM::CPropVariant propVariant;
    if (index < 0)
      name = s;
    else
    {
      name = s.Left(index);
      UString value = s.Mid(index + 1);
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
    const UString &options)
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
    try
    {
      Result = OutArchive->DoOperation(
        LibPath, &ClassID,
        OutArchivePath, ActionSetByte, 
        (SfxMode ? (const wchar_t *)SfxModule: NULL),
        UpdateCallback);
    }
    catch(const UString &s)
    {
      MyMessageBox(s);
      Result = E_FAIL;
    }
    catch(...)
    {
      Result = E_FAIL;
    }
    UpdateCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }

  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadUpdateCompress *)param)->Process();
  }
};

static UString MakeFullArchiveName(const UString &name, 
    const UString &extension, bool sfx)
{
  if (sfx)
  {
    UString sfxExt = L".exe";
    if (sfxExt.CollateNoCase(name.Right(sfxExt.Length())) == 0)
      return name;
    return name + sfxExt;
  }

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
  return name + UString(L'.') + extension;
}

HRESULT CompressArchive(
    const UString &archivePath, 
    const UStringVector &fileNames, 
    const UString &archiveType, 
    bool email,
    bool showDialog)
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

  UString tempDirPath;
  UString currentDirPrefix;
  bool needTempFile = true;
  NDirectory::CTempDirectoryW tempDirectory;
  UString archiveName;
  int pos = archivePath.ReverseFind(L'\\');
  if (pos < 0)
  {
    archiveName = archivePath;
    MyGetCurrentDirectory(currentDirPrefix);
  }
  else
  {
    currentDirPrefix = archivePath.Left(pos + 1);
    archiveName = archivePath.Mid(pos + 1);
  }
  
  if (email)
  {
    tempDirectory.Create(kTempFolderPrefix);
    currentDirPrefix = tempDirectory.GetPath();
    NormalizeDirPathPrefix(currentDirPrefix);
    needTempFile = false;
  }

  if (showDialog)
  {
    bool oneFile = false;
    NFind::CFileInfoW fileInfo;
    if (!NFind::FindFile(fileNames.Front(), fileInfo))
      return ::GetLastError();
    if (fileNames.Size() == 1)
      oneFile = !fileInfo.IsDirectory();

    CCompressDialog dialog;
    for(int i = 0; i < archivers.Size(); i++)
    {
      const CArchiverInfo &archiverInfo = archivers[i];
      if (archiverInfo.UpdateEnabled && 
        (oneFile || !archiverInfo.KeepName))
        dialog.m_ArchiverInfoList.Add(archiverInfo);
    }
    if(dialog.m_ArchiverInfoList.Size() == 0)
    {
      MyMessageBox(L"No Update Engines");
      return E_FAIL;
    }
    dialog.m_Info.ArchiveName = archiveName;
    dialog.OriginalFileName = fileInfo.Name;
 
    dialog.m_Info.CurrentDirPrefix = currentDirPrefix;
    dialog.m_Info.SFXMode = false;
    dialog.m_Info.Solid = true;
    dialog.m_Info.MultiThread = false;

    dialog.m_Info.KeepName = !oneFile;
    
    if(dialog.Create(0) != IDOK)
      return S_OK;
    
    if (dialog.m_Info.VolumeSizeIsDefined)
    {
      MyMessageBox(L"Splitting to volumes is not supported");
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
    compressInfo.ArchiveName = MakeFullArchiveName(
        compressInfo.ArchiveName, 
        archiverInfo.GetMainExtension(), compressInfo.SFXMode);
  }
  else
  {
    int i;
    for(i = 0; i < archivers.Size(); i++)
    {
      if (archivers[i].Name.CollateNoCase(archiveType) == 0)
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
    bool is7z = (archiveType.CollateNoCase(L"7z") == 0);
    compressInfo.SolidIsAllowed = is7z;
    compressInfo.Solid = true;
    compressInfo.MultiThreadIsAllowed = is7z;
    compressInfo.MultiThread = false;
    compressInfo.SFXMode = false;
    compressInfo.KeepName = false;
    compressInfo.ArchiveName = archiveName;
    compressInfo.CurrentDirPrefix = currentDirPrefix;
    compressInfo.Method = 5;
  }
  UString arcPath;
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
  UString workDir = GetWorkDir(workDirInfo, arcPath);
  NFile::NDirectory::CreateComplexDirectory(workDir);

  NFile::NDirectory::CTempFileW tempFile;
  UString tempFileName;
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
  NFind::CFileInfoW fileInfo;

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

  int i;
  for(i = 0; i < fileNames.Size(); i++)
    fileNamePointers.Add(fileNames[i]);

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

  UString title = LangLoadStringW(IDS_PROGRESS_COMPRESSING, 0x02000DC0);
  updater.UpdateCallbackSpec->Init(0, !password.IsEmpty(), password);

  UINT32 method = MyMin(compressInfo.Method, UINT32(9));
  HRESULT result = SetOutProperties(outArchive, 
      method, 
      compressInfo.SolidIsAllowed, compressInfo.Solid, 
      compressInfo.MultiThreadIsAllowed, compressInfo.MultiThread, 
      encryptHeadersIsAllowed, encryptHeaders,
      compressInfo.SFXMode,
      GetUnicodeString(compressInfo.Options));

  if (result != S_OK)
  {
    if (result != E_ABORT)
      ShowErrorMessage(result);
    return result;
  }


  UString sfxModule;
  if (compressInfo.SFXMode)
  {
    UString sfxModule2;
    LPCWSTR path = NULL;
    UString sfxModule3;
    if (GetProgramFolderPath(sfxModule3))
      path = sfxModule3;
    if (!NDirectory::MySearchPath(path, kDefaultSfxModule, NULL, sfxModule2))
    {
      MyMessageBox(L"can't find sfx module");
      return E_FAIL;
    }
    sfxModule = sfxModule2;
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
    if (!NDirectory::MyMoveFile(tempFileName, arcPath))
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

    UString fileName;
    GetOnlyName(arcPath, fileName);
    AString path = GetAnsiString(arcPath);
    AString name = GetAnsiString(fileName);
    fnSend(0, ";", (LPSTR)(LPCSTR)path, (LPSTR)(LPCSTR)name, 0); 
  }

  return S_OK;
}
