// PanelExtract.cpp

#include "StdAfx.h"

#include "Windows/COM.h"

#include "Panel.h"
#include "resource.h" 
#include "LangUtils.h"
#include "ExtractCallback.h"
#include "Windows/Thread.h"
////////////////////////////////////////////////////////////////

#include "..\UI\Resource\Extract\resource.h"

#include "UpdateCallback100.h"

using namespace NWindows;

struct CThreadExtractInArchive2
{
  CMyComPtr<IFolderOperations> FolderOperations;
  CRecordVector<UInt32> Indices;
  UString DestPath;
  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IFolderOperationsExtractCallback> ExtractCallback;
  HRESULT Result;
  bool MoveMode;

  CThreadExtractInArchive2(): MoveMode(false) {}
  
  DWORD Extract()
  {
    // NCOM::CComInitializer comInitializer;
    ExtractCallbackSpec->ProgressDialog.WaitCreating();
    if (MoveMode)
      Result = FolderOperations->MoveTo(&Indices.Front(), Indices.Size(), 
          DestPath, ExtractCallback);
    else
      Result = FolderOperations->CopyTo(&Indices.Front(), Indices.Size(), 
          DestPath, ExtractCallback);
    ExtractCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }
  
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadExtractInArchive2 *)param)->Extract();
  }
};

HRESULT CPanel::CopyTo(const CRecordVector<UInt32> &indices, const UString &folder, 
    bool moveMode, bool showErrorMessages, UStringVector *messages)
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    UString errorMessage = LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    if (showErrorMessages)
      MessageBox(errorMessage);
    else if (messages != 0)
      messages->Add(errorMessage);
    return E_FAIL;
  }

  CThreadExtractInArchive2 extracter;

  extracter.ExtractCallbackSpec = new CExtractCallbackImp;
  extracter.ExtractCallback = extracter.ExtractCallbackSpec;
  extracter.ExtractCallbackSpec->ParentWindow = GetParent();
  extracter.ExtractCallbackSpec->ShowMessages = showErrorMessages;

  UString title = moveMode ? 
      LangLoadStringW(IDS_MOVING, 0x03020206):
      LangLoadStringW(IDS_COPYING, 0x03020205);
  UString progressWindowTitle = LangLoadStringW(IDS_APP_TITLE, 0x03000000);

  extracter.ExtractCallbackSpec->ProgressDialog.MainWindow = GetParent();
  extracter.ExtractCallbackSpec->ProgressDialog.MainTitle = progressWindowTitle;
  extracter.ExtractCallbackSpec->ProgressDialog.MainAddTitle = title + L" ";

  extracter.ExtractCallbackSpec->OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
  extracter.ExtractCallbackSpec->Init();
  extracter.Indices = indices;
  extracter.DestPath = folder;
  extracter.FolderOperations = folderOperations;
  extracter.MoveMode = moveMode;

  CThread extractThread;
  if (!extractThread.Create(CThreadExtractInArchive2::MyThreadFunction, &extracter))
    throw 271824;
  extracter.ExtractCallbackSpec->StartProgressDialog(title);

  if (messages != 0)
    *messages = extracter.ExtractCallbackSpec->Messages;
  return extracter.Result;
}


struct CThreadUpdate
{
  CMyComPtr<IFolderOperations> FolderOperations;
  UString FolderPrefix;
  UStringVector FileNames;
  CRecordVector<const wchar_t *> FileNamePointers;
  CMyComPtr<IFolderArchiveUpdateCallback> UpdateCallback;
  CUpdateCallback100Imp *UpdateCallbackSpec;
  HRESULT Result;
  
  DWORD Process()
  {
    NCOM::CComInitializer comInitializer;
    UpdateCallbackSpec->ProgressDialog.WaitCreating();
    Result = FolderOperations->CopyFrom(
        FolderPrefix,
        &FileNamePointers.Front(),
        FileNamePointers.Size(),
        UpdateCallback);
    UpdateCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }

  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadUpdate *)param)->Process();
  }
};


HRESULT CPanel::CopyFrom(const UString &folderPrefix, const UStringVector &filePaths, 
    bool showErrorMessages, UStringVector *messages)
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    UString errorMessage = LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    if (showErrorMessages)
      MessageBox(errorMessage);
    else if (messages != 0)
      messages->Add(errorMessage);
    return E_FAIL;
  }

  CThreadUpdate updater;
  updater.UpdateCallbackSpec = new CUpdateCallback100Imp;
  updater.UpdateCallback = updater.UpdateCallbackSpec;

  UString title = LangLoadStringW(IDS_COPYING, 0x03020205);
  UString progressWindowTitle = LangLoadStringW(IDS_APP_TITLE, 0x03000000);

  updater.UpdateCallbackSpec->ProgressDialog.MainWindow = GetParent();
  updater.UpdateCallbackSpec->ProgressDialog.MainTitle = progressWindowTitle;
  updater.UpdateCallbackSpec->ProgressDialog.MainAddTitle = title + UString(L" ");
  
  updater.UpdateCallbackSpec->Init((HWND)*this, false, L"");
  updater.FolderOperations = folderOperations;
  updater.FolderPrefix = folderPrefix;
  updater.FileNames.Reserve(filePaths.Size());
  int i;
  for(i = 0; i < filePaths.Size(); i++)
    updater.FileNames.Add(filePaths[i]);
  updater.FileNamePointers.Reserve(updater.FileNames.Size());
  for(i = 0; i < updater.FileNames.Size(); i++)
    updater.FileNamePointers.Add(updater.FileNames[i]);

  CThread thread;
  if (!thread.Create(CThreadUpdate::MyThreadFunction, &updater))
    throw 271824;
  updater.UpdateCallbackSpec->StartProgressDialog(title);
  
  if (messages != 0)
    *messages = updater.UpdateCallbackSpec->Messages;

  return updater.Result;
}

void CPanel::CopyFrom(const UStringVector &filePaths)
{
  UString message = L"Are you sure you want to copy files to archive\n\'";
  message += _currentFolderPrefix;
  message += L"\' ?";
  int res = ::MessageBoxW(*(this), message, L"Confirm File Copy", 
    MB_YESNOCANCEL | MB_ICONQUESTION | MB_TASKMODAL);
  if (res != IDYES)
    return;

  CDisableTimerProcessing disableTimerProcessing(*this);

  CSelectedState srcSelState;
  SaveSelectedState(srcSelState);

  HRESULT result = CopyFrom(L"", filePaths, true, 0);

  if (result != S_OK)
  {
    disableTimerProcessing.Restore();
    // For Password:
    SetFocusToList();
    if (result != E_ABORT)
      MessageBoxError(result, L"Error");
    return;
  }

  RefreshListCtrl(srcSelState);

  disableTimerProcessing.Restore();
  SetFocusToList();
}
