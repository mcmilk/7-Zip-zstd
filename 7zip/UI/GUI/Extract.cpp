// Extract.h

#include "StdAfx.h"

#include "Extract.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#ifndef EXCLUDE_COM
#include "Windows/DLL.h"
#endif
#include "Windows/Thread.h"

#include "../Common/OpenArchive.h"
#include "../Common/DefaultName.h"

#ifndef EXCLUDE_COM
#include "../Common/ZipRegistry.h"
#endif

#include "../Resource/Extract/resource.h"

#include "../Explorer/MyMessages.h"
#include "../../FileManager/FormatUtils.h"

#include "ExtractDialog.h"
#include "../../FileManager/ExtractCallback.h"

#include "../Agent/ArchiveExtractCallback.h"

#include "../../FileManager/OpenCallback.h"

using namespace NWindows;

struct CThreadExtracting
{
  #ifndef EXCLUDE_COM
  NDLL::CLibrary Library;
  #endif
  CMyComPtr<IInArchive> Archive;
  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IFolderArchiveExtractCallback> ExtractCallback2;
  CMyComPtr<IArchiveExtractCallback> ArchiveExtractCallback;

  HRESULT Result;
  
  DWORD Process()
  {
    ExtractCallbackSpec->ProgressDialog.WaitCreating();
    Result = Archive->Extract(0, -1, BoolToInt(false), 
        ArchiveExtractCallback);
    ExtractCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadExtracting *)param)->Process();
  }
};

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

HRESULT ExtractArchive(HWND parentWindow, const CSysString &fileName, 
    bool assumeYes, bool showDialog, const CSysString &outputFolder)
{
  CThreadExtracting extracter;

  CArchiverInfo archiverInfo;

  COpenArchiveCallback *openCallbackSpec = new COpenArchiveCallback;
  CMyComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
  openCallbackSpec->_passwordIsDefined = false;
  openCallbackSpec->_parentWindow = parentWindow;

  CSysString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(fileName, fullName, fileNamePartStartIndex);

  openCallbackSpec->LoadFileInfo(
      fullName.Left(fileNamePartStartIndex), 
      fullName.Mid(fileNamePartStartIndex));

  int subExtIndex;
  HRESULT res = OpenArchive(fileName, 
      #ifndef EXCLUDE_COM
      &extracter.Library, 
      #endif
      &extracter.Archive, archiverInfo, subExtIndex, openCallback);
  RINOK(res);

  NFile::NFind::CFileInfo fileInfo;
  if (!NFile::NFind::FindFile(fileName, fileInfo))
    return E_FAIL;
  UString defaultName = GetDefaultName(fileName, 
      archiverInfo.Extensions[subExtIndex].Extension, 
      archiverInfo.Extensions[subExtIndex].AddExtension);

  CSysString directoryPath;
  NExtractionDialog::CModeInfo extractModeInfo;
  UString password;
  if (openCallbackSpec->_passwordIsDefined)
    password = openCallbackSpec->_password;
  if (showDialog)
  {
    CExtractDialog dialog;
    dialog.Init(
    #ifndef  NO_REGISTRY
      // &aZipRegistryManager, 
    #endif
      fileName);
    dialog._filesMode = NExtractionDialog::NFilesMode::kAll;
    dialog._enableSelectedFilesButton = false;
    dialog._enableFilesButton = false;
    dialog._password = GetSystemString(password);
    
    if(dialog.Create(parentWindow) != IDOK)
      return E_ABORT;
    directoryPath = dialog._directoryPath;
    dialog.GetModeInfo(extractModeInfo);

    password = GetUnicodeString((LPCTSTR)dialog._password);
  }
  else
  {
    /*
    CSysString fullPath;
    int fileNamePartStartIndex;
    if (!NWindows::NFile::NDirectory::MyGetFullPathName(fileName, fullPath, fileNamePartStartIndex))
    {
      MessageBox(NULL, TEXT("Error 1329484"), TEXT("7-Zip"), 0);
      return E_FAIL;
    }
    directoryPath = fullPath.Left(fileNamePartStartIndex);
    */
    // NFile::NDirectory::MyGetCurrentDirectory(directoryPath);
    if (!NFile::NDirectory::MyGetFullPathName(outputFolder, directoryPath))
    {
      MessageBox(NULL, TEXT("Error 1329484"), TEXT("7-Zip"), 0);
      return E_FAIL;
    }
    NFile::NName::NormalizeDirPathPrefix(directoryPath);

    extractModeInfo.PathMode = NExtractionDialog::NPathMode::kFullPathnames;
    extractModeInfo.OverwriteMode = assumeYes ?
      NExtractionDialog::NOverwriteMode::kWithoutPrompt:
      NExtractionDialog::NOverwriteMode::kAskBefore;
    extractModeInfo.FilesMode = NExtractionDialog::NFilesMode::kAll;
  }
  if(!NFile::NDirectory::CreateComplexDirectory(directoryPath))
  {
    MyMessageBox(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, 
        #ifdef LANG        
        0x02000603, 
        #endif 
        GetUnicodeString((LPCTSTR)directoryPath)));
    return E_FAIL;
  }
  
  extracter.ExtractCallbackSpec = new CExtractCallbackImp;

  extracter.ExtractCallback2 = extracter.ExtractCallbackSpec;
  
  extracter.ExtractCallbackSpec->_parentWindow = 0;
  #ifdef LANG        
  const CSysString title = LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890);
  #else
  const CSysString title = NWindows::MyLoadString(IDS_PROGRESS_EXTRACTING);
  #endif

  NFile::NFind::CFileInfo archiveFileInfo;
  if (!NFile::NFind::FindFile(fileName, archiveFileInfo))
    throw "there is no archive file";

  extracter.ExtractCallbackSpec->Init(NExtractionMode::NOverwrite::kAskBefore, 
      !password.IsEmpty(), password);

  NExtractionMode::NPath::EEnum pathMode;
  NExtractionMode::NOverwrite::EEnum overwriteMode;
  switch (extractModeInfo.OverwriteMode)
  {
    case NExtractionDialog::NOverwriteMode::kAskBefore:
      overwriteMode = NExtractionMode::NOverwrite::kAskBefore;
      break;
    case NExtractionDialog::NOverwriteMode::kWithoutPrompt:
      overwriteMode = NExtractionMode::NOverwrite::kWithoutPrompt;
      break;
    case NExtractionDialog::NOverwriteMode::kSkipExisting:
      overwriteMode = NExtractionMode::NOverwrite::kSkipExisting;
      break;
    case NExtractionDialog::NOverwriteMode::kAutoRename:
      overwriteMode = NExtractionMode::NOverwrite::kAutoRename;
      break;
    default:
      throw 12334454;
  }
  switch (extractModeInfo.PathMode)
  {
    case NExtractionDialog::NPathMode::kFullPathnames:
      pathMode = NExtractionMode::NPath::kFullPathnames;
      break;
    case NExtractionDialog::NPathMode::kCurrentPathnames:
      pathMode = NExtractionMode::NPath::kCurrentPathnames;
      break;
    case NExtractionDialog::NPathMode::kNoPathnames:
      pathMode = NExtractionMode::NPath::kNoPathnames;
      break;
    default:
      throw 12334455;
  }

  CArchiveExtractCallback *extractCallbackSpec = new 
      CArchiveExtractCallback;
  extracter.ArchiveExtractCallback = extractCallbackSpec;

  extractCallbackSpec->Init(extracter.Archive, 
      extracter.ExtractCallback2, 
      directoryPath, pathMode, 
      overwriteMode, UStringVector(),
      GetCurrentFileCodePage(), 
      defaultName, 
      fileInfo.LastWriteTime, fileInfo.Attributes);

  CThread thread;
  if (!thread.Create(CThreadExtracting::MyThreadFunction, &extracter))
    throw 271824;
  extracter.ExtractCallbackSpec->StartProgressDialog(title);
  return extracter.Result;
}



