// ExtractEngine.h

#include "StdAfx.h"

#include "ExtractEngine.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"

#include "../Common/OpenEngine2.h"

#ifndef  NO_REGISTRY
#include "../Common/ZipRegistry.h"
#endif

#include "../Resource/Extract/resource.h"

#include "MyMessages.h"
#include "../../FileManager/FormatUtils.h"

#include "ExtractDialog.h"
#include "../../FileManager/ExtractCallback.h"

using namespace NWindows;

HRESULT ExtractArchive(HWND parentWindow, const CSysString &fileName, 
    bool assumeYes)
{
  CComPtr<IArchiveHandler100> archiveHandler;
  NZipRootRegistry::CArchiverInfo archiverInfoResult;

  UString defaultName;
  HRESULT result = OpenArchive(fileName, &archiveHandler, 
      archiverInfoResult, defaultName, NULL);
  if (result != S_OK)
    return result;

  CSysString directoryPath;
  NExtractionDialog::CModeInfo extractModeInfo;
  UString password;
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
  
  CComObjectNoLock<CExtractCallbackImp> *extractCallBackSpec =
    new CComObjectNoLock<CExtractCallbackImp>;

  CComPtr<IExtractCallback2> extractCallBack(extractCallBackSpec);
  
  extractCallBackSpec->_parentWindow = 0;
  #ifdef LANG        
  const CSysString title = LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890);
  #else
  const CSysString title = NWindows::MyLoadString(IDS_PROGRESS_EXTRACTING);
  #endif
  extractCallBackSpec->StartProgressDialog(title);

  // extractCallBackSpec->m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);

  UStringVector aRemovePathParts;

  NFile::NFind::CFileInfo archiveFileInfo;
  if (!NFile::NFind::FindFile(fileName, archiveFileInfo))
    throw "there is no archive file";

  extractCallBackSpec->Init(NExtractionMode::NOverwrite::kAskBefore, 
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

  return archiveHandler->Extract(
      pathMode, overwriteMode, GetUnicodeString(directoryPath), 
      BoolToMyBool(false), extractCallBack);
}



