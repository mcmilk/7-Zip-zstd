// ZipViewExtract.cpp

#include "StdAfx.h"

#include "ZipViewObject.h"
#include "ZipViewUtils.h"

#include "ExtractCallback.h"

#include "Windows/FileDir.h"
#include "Windows/ResourceString.h"

#include "..\..\Archiver\Common\DefaultName.h"
#include "Common/StringConvert.h"
#include "../Resource/Extract/resource.h"

#include "FormatUtils.h"

using namespace NWindows;

void CZipViewObject::CommandExtract()
{
  HWND aBrowserWindow;
  if(m_ShellBrowser->GetWindow(&aBrowserWindow) != S_OK)
    return;

  CRecordVector<UINT32> aLocalIndexes;
  if(::GetFocus() == HWND(m_ListView))
    GetSelectedItemsIndexes(aLocalIndexes);
  
  CZipRegistryManager aZipRegistryManager;
  CExtractDialog aDialog;
  aDialog.Init(&aZipRegistryManager, GetSystemString(m_ZipFolder->m_FileName));
  aDialog.m_EnableFilesButton = false;
  if(aLocalIndexes.Size() == 0)
  {
    aDialog.m_FilesMode = NExtractionDialog::NFilesMode::kAll;
    aDialog.m_EnableSelectedFilesButton = false;
  }
  else
  {
    aDialog.m_FilesMode = NExtractionDialog::NFilesMode::kSelected;
    aDialog.m_EnableSelectedFilesButton = true;
  }
  /*
  if (!NFile::NDirectory::GetOnlyDirPrefix(aFileName, aDialog.m_DirectoryPath))
  {
    ShowLastErrorMessage();
    return E_FAIL;
  }
  */
  if(aDialog.Create(m_Window) != IDOK)
    return;
  CSysString aDirectoryPath = aDialog.m_DirectoryPath;
  if(!NFile::NDirectory::CreateComplexDirectory(aDirectoryPath))
  {
    CSysString aString = 
        MyFormat(IDS_CANNOT_CREATE_FOLDER, 0x02000603, (LPCTSTR)aDirectoryPath);
    MessageBox(aString);
    return;
  }
  NExtractionDialog::CModeInfo anExtractModeInfo;
  aDialog.GetModeInfo(anExtractModeInfo);
  UString aPassword = GetUnicodeString((LPCTSTR)aDialog.m_Password);
  ExtractItems(anExtractModeInfo, aDirectoryPath, aLocalIndexes, !aPassword.IsEmpty(), aPassword);
}
  
 
HRESULT CZipViewObject::ExtractItems(
    const NExtractionDialog::CModeInfo &anExtractModeInfo,
    const CSysString &aDirectoryPath,
    const CRecordVector<UINT32> &aLocalIndexes,
    bool aPasswordIsDefined, const UString &aPassword)
{
  CComObjectNoLock<CExtractCallBackImp> *anExtractCallBackSpec =
    new CComObjectNoLock<CExtractCallBackImp>;
  CComPtr<IExtractCallback2> anExtractCallBack(anExtractCallBackSpec);
  
  anExtractCallBackSpec->m_ParentWindow = m_Window;
  CShellBrowserDisabler aWndEnabledRestorer(m_ShellBrowser);

  #ifdef LANG        
  const CSysString aTitle = LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890);
  #else
  const CSysString aTitle = NWindows::MyLoadString(IDS_PROGRESS_EXTRACTING);
  #endif
  anExtractCallBackSpec->StartProgressDialog(aTitle);
  
  anExtractCallBackSpec->Init(NExtractionMode::NOverwrite::kAskBefore, 
      aPasswordIsDefined, aPassword);

  NExtractionMode::NPath::EEnum aPathMode;
  NExtractionMode::NOverwrite::EEnum anOverwriteMode;
  switch (anExtractModeInfo.OverwriteMode)
  {
    case NExtractionDialog::NOverwriteMode::kAskBefore:
      anOverwriteMode = NExtractionMode::NOverwrite::kAskBefore;
      break;
    case NExtractionDialog::NOverwriteMode::kWithoutPrompt:
      anOverwriteMode = NExtractionMode::NOverwrite::kWithoutPrompt;
      break;
    case NExtractionDialog::NOverwriteMode::kSkipExisting:
      anOverwriteMode = NExtractionMode::NOverwrite::kSkipExisting;
      break;
    default:
      throw 12334454;
  }
  switch (anExtractModeInfo.PathMode)
  {
    case NExtractionDialog::NPathMode::kFullPathnames:
      aPathMode = NExtractionMode::NPath::kFullPathnames;
      break;
    case NExtractionDialog::NPathMode::kCurrentPathnames:
      aPathMode = NExtractionMode::NPath::kCurrentPathnames;
      break;
    case NExtractionDialog::NPathMode::kNoPathnames:
      aPathMode = NExtractionMode::NPath::kNoPathnames;
      break;
    default:
      throw 12334455;
  }

  if(anExtractModeInfo.FilesMode == NExtractionDialog::NFilesMode::kSelected)
  {
    // vector<int> aRealIndexes;
    // AddAllRealIndexes(aLocalIndexes, aRealIndexes);
    return m_ArchiveFolder->Extract(&aLocalIndexes.Front(),
        aLocalIndexes.Size(), 
        aPathMode, anOverwriteMode,
        GetUnicodeString(aDirectoryPath), BoolToMyBool(false), 
        anExtractCallBack);
  }
  else // All
  {
    return m_ZipFolder->m_ArchiveHandler->Extract(aPathMode, anOverwriteMode,
        GetUnicodeString(aDirectoryPath), BoolToMyBool(false), 
        anExtractCallBack);
  }
}
