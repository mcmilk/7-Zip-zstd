// TestEngine.h

#include "StdAfx.h"

#include "TestEngine.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"

#include "../Common/OpenEngine2.h"

#ifndef  NO_REGISTRY
#include "../Common/ZipRegistry.h"
#endif

#include "MyMessages.h"
#include "FormatUtils.h"

#include "ExtractCallback.h"

using namespace NWindows;

HRESULT TestArchive(HWND aParentWindow, const CSysString &aFileName)
{
  CComPtr<IArchiveHandler100> anArchiveHandler;
  NZipRootRegistry::CArchiverInfo anArchiverInfoResult;

  UString aDefaultName;
  HRESULT aResult = OpenArchive(aFileName, &anArchiveHandler, 
      anArchiverInfoResult, aDefaultName, NULL);
  if (aResult != S_OK)
    return aResult;

  CComObjectNoLock<CExtractCallBackImp> *anExtractCallBackSpec =
    new CComObjectNoLock<CExtractCallBackImp>;

  CComPtr<IExtractCallback2> anExtractCallBack(anExtractCallBackSpec);
  
  anExtractCallBackSpec->m_ParentWindow = 0;
  anExtractCallBackSpec->StartProgressDialog(true);

  // anExtractCallBackSpec->m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);

  UString aPassword;

  NFile::NFind::CFileInfo anArchiveFileInfo;
  if (!NFile::NFind::FindFile(aFileName, anArchiveFileInfo))
    throw "there is no archive file";

  NExtractionDialog::CModeInfo anExtractModeInfo;
  anExtractModeInfo.OverwriteMode = NExtractionDialog::NOverwriteMode::kAskBefore;
  anExtractModeInfo.PathMode = NExtractionDialog::NPathMode::kFullPathnames;
  anExtractModeInfo.FilesMode = NExtractionDialog::NFilesMode::kAll;
  anExtractModeInfo.FileList.Clear();

  anExtractCallBackSpec->Init(anArchiveHandler, anExtractModeInfo,
      !aPassword.IsEmpty(), aPassword);

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
    case NExtractionDialog::NOverwriteMode::kAutoRename:
      anOverwriteMode = NExtractionMode::NOverwrite::kAutoRename;
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

  aResult = anArchiveHandler->Extract(
      aPathMode, anOverwriteMode, L"", 
      true, anExtractCallBack);

  if (anExtractCallBackSpec->m_Messages.IsEmpty())
  {
    anExtractCallBackSpec->DestroyWindows();
    MessageBox(0, LangLoadString(IDS_MESSAGE_NO_ERRORS, 0x02000608),
      LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90), 0);
  }
  return aResult;
}
