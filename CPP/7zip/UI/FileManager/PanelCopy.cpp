// PanelExtract.cpp

#include "StdAfx.h"

#include "Panel.h"
#include "resource.h" 
#include "LangUtils.h"
#include "ExtractCallback.h"
#include "Windows/Thread.h"
////////////////////////////////////////////////////////////////

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
  
  static THREAD_FUNC_DECL MyThreadFunction(void *param)
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
    UString errorMessage = LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    if (showErrorMessages)
      MessageBox(errorMessage);
    else if (messages != 0)
      messages->Add(errorMessage);
    return E_FAIL;
  }

  HRESULT res;
  {
  CThreadExtractInArchive2 extracter;
  
  extracter.ExtractCallbackSpec = new CExtractCallbackImp;
  extracter.ExtractCallback = extracter.ExtractCallbackSpec;
  extracter.ExtractCallbackSpec->ParentWindow = GetParent();
  extracter.ExtractCallbackSpec->ShowMessages = showErrorMessages;
  extracter.ExtractCallbackSpec->ProgressDialog.CompressingMode = false;
  
  UString title = moveMode ? 
      LangString(IDS_MOVING, 0x03020206):
      LangString(IDS_COPYING, 0x03020205);
  UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);
  
  extracter.ExtractCallbackSpec->ProgressDialog.MainWindow = GetParent();
  extracter.ExtractCallbackSpec->ProgressDialog.MainTitle = progressWindowTitle;
  extracter.ExtractCallbackSpec->ProgressDialog.MainAddTitle = title + L" ";
    
  extracter.ExtractCallbackSpec->OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
  extracter.ExtractCallbackSpec->Init();
  extracter.Indices = indices;
  extracter.DestPath = folder;
  extracter.FolderOperations = folderOperations;
  extracter.MoveMode = moveMode;
  
  NWindows::CThread extractThread;
  RINOK(extractThread.Create(CThreadExtractInArchive2::MyThreadFunction, &extracter));
  extracter.ExtractCallbackSpec->StartProgressDialog(title);
  
  if (messages != 0)
    *messages = extracter.ExtractCallbackSpec->Messages;
  res = extracter.Result; 
  }
  RefreshTitleAlways();
  return res;
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
    UpdateCallbackSpec->ProgressDialog.WaitCreating();
    Result = FolderOperations->CopyFrom(
        FolderPrefix,
        &FileNamePointers.Front(),
        FileNamePointers.Size(),
        UpdateCallback);
    UpdateCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }

  static THREAD_FUNC_DECL MyThreadFunction(void *param)
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
    UString errorMessage = LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    if (showErrorMessages)
      MessageBox(errorMessage);
    else if (messages != 0)
      messages->Add(errorMessage);
    return E_FAIL;
  }

  HRESULT res;
  {
  CThreadUpdate updater;
  updater.UpdateCallbackSpec = new CUpdateCallback100Imp;
  updater.UpdateCallback = updater.UpdateCallbackSpec;

  UString title = LangString(IDS_COPYING, 0x03020205);
  UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);

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

  NWindows::CThread thread;
  RINOK(thread.Create(CThreadUpdate::MyThreadFunction, &updater));
  updater.UpdateCallbackSpec->StartProgressDialog(title);
  
  if (messages != 0)
    *messages = updater.UpdateCallbackSpec->Messages;

  res = updater.Result;
  }
  RefreshTitleAlways();
  return res;
}

void CPanel::CopyFromNoAsk(const UStringVector &filePaths)
{
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
      MessageBoxError(result);
    return;
  }

  RefreshListCtrl(srcSelState);

  disableTimerProcessing.Restore();
  SetFocusToList();
}

void CPanel::CopyFromAsk(const UStringVector &filePaths)
{
  UString title = LangString(IDS_CONFIRM_FILE_COPY, 0x03020222);
  UString message = LangString(IDS_WANT_TO_COPY_FILES, 0x03020223);
  message += L"\n\'";
  message += _currentFolderPrefix;
  message += L"\' ?";
  int res = ::MessageBoxW(*(this), message, title, MB_YESNOCANCEL | MB_ICONQUESTION | MB_SYSTEMMODAL);
  if (res != IDYES)
    return;

  CopyFromNoAsk(filePaths);
}

