// ExtractEngine.h

#include "StdAfx.h"

#include "ExtractEngine.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/COM.h"
#include "Windows/Thread.h"

#include "../Common/OpenEngine200.h"
#include "../Common/DefaultName.h"

#ifndef  NO_REGISTRY
#include "../Common/ZipRegistry.h"
#endif

#include "../Resource/Extract/resource.h"

#include "MyMessages.h"
#include "../../FileManager/FormatUtils.h"

#include "ExtractDialog.h"
#include "../../FileManager/ExtractCallback.h"

#include "../Agent/ArchiveExtractCallback.h"

#include "../../FileManager/OpenCallback.h"

using namespace NWindows;

struct CThreadExtracting
{
  CComPtr<IInArchive> Archive;
  CComObjectNoLock<CExtractCallbackImp> *ExtractCallbackSpec;
  CComPtr<IFolderArchiveExtractCallback> ExtractCallback2;
  CComPtr<IArchiveExtractCallback> ArchiveExtractCallback;

  HRESULT Result;
  
  DWORD Process()
  {
    NCOM::CComInitializer comInitializer;
    ExtractCallbackSpec->_progressDialog._dialogCreatedEvent.Lock();
    #ifndef _SFX
    ExtractCallbackSpec->_appTitle.Window = (HWND)ExtractCallbackSpec->_progressDialog;
    #endif
    Result = Archive->ExtractAllItems(BoolToInt(false), 
        ArchiveExtractCallback);
    ExtractCallbackSpec->_progressDialog.MyClose();
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
    bool assumeYes)
{
  CThreadExtracting extracter;

  NZipRootRegistry::CArchiverInfo archiverInfo;

  CComObjectNoLock<COpenArchiveCallback> *openCallbackSpec = 
      new CComObjectNoLock<COpenArchiveCallback>;
  CComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
  openCallbackSpec->_passwordIsDefined = false;
  openCallbackSpec->_parentWindow = parentWindow;
  RETURN_IF_NOT_S_OK(OpenArchive(fileName, &extracter.Archive, 
      archiverInfo, openCallback));

  NFile::NFind::CFileInfo fileInfo;
  if (!NFile::NFind::FindFile(fileName, fileInfo))
    return E_FAIL;
  UString defaultName = GetDefaultName(fileName, archiverInfo.Extension, 
      GetUnicodeString(archiverInfo.AddExtension));

  CSysString directoryPath;
  NExtractionDialog::CModeInfo extractModeInfo;
  UString password;
  if (openCallbackSpec->_passwordIsDefined)
    password = openCallbackSpec->_password;
  if (!assumeYes)
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
    CSysString aFullPath;
    int aFileNamePartStartIndex;
    if (!NWindows::NFile::NDirectory::MyGetFullPathName(fileName, aFullPath, aFileNamePartStartIndex))
    {
      MessageBox(NULL, TEXT("Error 1329484"), TEXT("7-Zip"), 0);
      return E_FAIL;
    }
    directoryPath = aFullPath.Left(aFileNamePartStartIndex);
    extractModeInfo.PathMode = NExtractionDialog::NPathMode::kFullPathnames;
    extractModeInfo.OverwriteMode = NExtractionDialog::NOverwriteMode::kWithoutPrompt;
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
  
  extracter.ExtractCallbackSpec = new CComObjectNoLock<CExtractCallbackImp>;

  extracter.ExtractCallback2 = extracter.ExtractCallbackSpec;
  
  extracter.ExtractCallbackSpec->_parentWindow = 0;
  #ifdef LANG        
  const CSysString title = LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890);
  #else
  const CSysString title = NWindows::MyLoadString(IDS_PROGRESS_EXTRACTING);
  #endif
  #ifndef _SFX
  // extracter.ExtractCallbackSpec->_appTitle.Window = (HWND)extracter.ExtractCallbackSpec->_progressDialog;
  extracter.ExtractCallbackSpec->_appTitle.Window = (HWND)0;
  extracter.ExtractCallbackSpec->_appTitle.Title = title;
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

  CComObjectNoLock<CArchiveExtractCallback> *extractCallbackSpec = new 
      CComObjectNoLock<CArchiveExtractCallback>;
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



