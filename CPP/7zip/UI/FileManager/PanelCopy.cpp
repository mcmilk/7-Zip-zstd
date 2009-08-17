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

class CPanelCopyThread: public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  CMyComPtr<IFolderOperations> FolderOperations;
  CRecordVector<UInt32> Indices;
  UString DestPath;
  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IFolderOperationsExtractCallback> ExtractCallback;
  HRESULT Result;
  bool MoveMode;

  CPanelCopyThread(): MoveMode(false), Result(E_FAIL) {}
};
  
HRESULT CPanelCopyThread::ProcessVirt()
{
  if (MoveMode)
    Result = FolderOperations->MoveTo(&Indices.Front(), Indices.Size(), DestPath, ExtractCallback);
  else
    Result = FolderOperations->CopyTo(&Indices.Front(), Indices.Size(), DestPath, ExtractCallback);
  return Result;
}

HRESULT CPanel::CopyTo(const CRecordVector<UInt32> &indices, const UString &folder,
    bool moveMode, bool showErrorMessages, UStringVector *messages,
    bool &usePassword, UString &password)
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
  CPanelCopyThread extracter;

  
  extracter.ExtractCallbackSpec = new CExtractCallbackImp;
  extracter.ExtractCallback = extracter.ExtractCallbackSpec;
  extracter.ExtractCallbackSpec->ProgressDialog = &extracter.ProgressDialog;
  extracter.ProgressDialog.CompressingMode = false;
  
  UString title = moveMode ?
      LangString(IDS_MOVING, 0x03020206):
      LangString(IDS_COPYING, 0x03020205);
  UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);
  
  extracter.ProgressDialog.MainWindow = GetParent();
  extracter.ProgressDialog.MainTitle = progressWindowTitle;
  extracter.ProgressDialog.MainAddTitle = title + L" ";
    
  extracter.ExtractCallbackSpec->OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
  extracter.ExtractCallbackSpec->Init();
  extracter.Indices = indices;
  extracter.DestPath = folder;
  extracter.FolderOperations = folderOperations;
  extracter.MoveMode = moveMode;

  extracter.ExtractCallbackSpec->PasswordIsDefined = usePassword;
  extracter.ExtractCallbackSpec->Password = password;
  
  RINOK(extracter.Create(title, GetParent()));
  
  if (messages != 0)
    *messages = extracter.ProgressDialog.Sync.Messages;
  res = extracter.Result;

  if (res == S_OK && extracter.ExtractCallbackSpec->IsOK())
  {
    usePassword = extracter.ExtractCallbackSpec->PasswordIsDefined;
    password = extracter.ExtractCallbackSpec->Password;
  }
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
  CProgressDialog ProgressDialog;
  CMyComPtr<IFolderArchiveUpdateCallback> UpdateCallback;
  CUpdateCallback100Imp *UpdateCallbackSpec;
  HRESULT Result;
  
  void Process()
  {
    try
    {
      CProgressCloser closer(ProgressDialog);
      Result = FolderOperations->CopyFrom(
        FolderPrefix,
        &FileNamePointers.Front(),
        FileNamePointers.Size(),
        UpdateCallback);
    }
    catch(...) { Result = E_FAIL; }
  }
  static THREAD_FUNC_DECL MyThreadFunction(void *param)
  {
    ((CThreadUpdate *)param)->Process();
    return 0;
  }
};

HRESULT CPanel::CopyFrom(const UString &folderPrefix, const UStringVector &filePaths,
    bool showErrorMessages, UStringVector *messages)
{
  CMyComPtr<IFolderOperations> folderOperations;
  _folder.QueryInterface(IID_IFolderOperations, &folderOperations);
  HRESULT res;
  if (!folderOperations)
    res = E_NOINTERFACE;
  else
  {
  CThreadUpdate updater;
  updater.UpdateCallbackSpec = new CUpdateCallback100Imp;
  updater.UpdateCallback = updater.UpdateCallbackSpec;

  updater.UpdateCallbackSpec->ProgressDialog = &updater.ProgressDialog;

  UString title = LangString(IDS_COPYING, 0x03020205);
  UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);

  updater.ProgressDialog.MainWindow = GetParent();
  updater.ProgressDialog.MainTitle = progressWindowTitle;
  updater.ProgressDialog.MainAddTitle = title + UString(L" ");
  
  updater.UpdateCallbackSpec->Init(false, L"");
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
  updater.ProgressDialog.Create(title, thread, GetParent());
  
  if (messages != 0)
    *messages = updater.ProgressDialog.Sync.Messages;

  res = updater.Result;
  }

  if (res == E_NOINTERFACE)
  {
    UString errorMessage = LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    if (showErrorMessages)
      MessageBox(errorMessage);
    else if (messages != 0)
      messages->Add(errorMessage);
    return E_ABORT;
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
  int res = ::MessageBoxW(*(this), message, title, MB_YESNOCANCEL | MB_ICONQUESTION);
  if (res != IDYES)
    return;

  CopyFromNoAsk(filePaths);
}
