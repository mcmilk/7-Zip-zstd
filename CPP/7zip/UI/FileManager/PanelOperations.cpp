// PanelOperations.cpp

#include "StdAfx.h"

#include "Common/DynamicBuffer.h"
#include "Common/StringConvert.h"

#include "Windows/COM.h"
#include "Windows/FileDir.h"
#include "Windows/PropVariant.h"
#include "Windows/ResourceString.h"
#include "Windows/Thread.h"

#include "ComboDialog.h"

#include "FSFolder.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "Panel.h"
#include "UpdateCallback100.h"

#include "resource.h"

using namespace NWindows;
using namespace NFile;

#ifndef _UNICODE
extern bool g_IsNT;
#endif

enum EFolderOpType
{
  FOLDER_TYPE_CREATE_FOLDER = 0,
  FOLDER_TYPE_DELETE = 1,
  FOLDER_TYPE_RENAME = 2
};

class CThreadFolderOperations: public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  EFolderOpType OpType;
  UString Name;
  UInt32 Index;
  CRecordVector<UInt32> Indices;

  CMyComPtr<IFolderOperations> FolderOperations;
  CMyComPtr<IProgress> UpdateCallback;
  CUpdateCallback100Imp *UpdateCallbackSpec;
  
  HRESULT Result;

  CThreadFolderOperations(EFolderOpType opType): OpType(opType), Result(E_FAIL) {}
  HRESULT DoOperation(CPanel &panel, const UString &progressTitle, const UString &titleError);
};
  
HRESULT CThreadFolderOperations::ProcessVirt()
{
  NCOM::CComInitializer comInitializer;
  switch(OpType)
  {
    case FOLDER_TYPE_CREATE_FOLDER:
      Result = FolderOperations->CreateFolder(Name, UpdateCallback);
      break;
    case FOLDER_TYPE_DELETE:
      Result = FolderOperations->Delete(&Indices.Front(), Indices.Size(), UpdateCallback);
      break;
    case FOLDER_TYPE_RENAME:
      Result = FolderOperations->Rename(Index, Name, UpdateCallback);
      break;
    default:
      Result = E_FAIL;
  }
  return Result;
}


HRESULT CThreadFolderOperations::DoOperation(CPanel &panel, const UString &progressTitle, const UString &titleError)
{
  UpdateCallbackSpec = new CUpdateCallback100Imp;
  UpdateCallback = UpdateCallbackSpec;
  UpdateCallbackSpec->ProgressDialog = &ProgressDialog;

  ProgressDialog.WaitMode = true;
  ProgressDialog.Sync.SetErrorMessageTitle(titleError);
  Result = S_OK;

  bool usePassword = false;
  UString password;
  if (panel._parentFolders.Size() > 0)
  {
    const CFolderLink &fl = panel._parentFolders.Back();
    usePassword = fl.UsePassword;
    password = fl.Password;
  }

  UpdateCallbackSpec->Init(usePassword, password);

  ProgressDialog.MainWindow = panel._mainWindow; // panel.GetParent()
  ProgressDialog.MainTitle = LangString(IDS_APP_TITLE, 0x03000000);
  ProgressDialog.MainAddTitle = progressTitle + UString(L" ");

  RINOK(Create(progressTitle, ProgressDialog.MainWindow));
  return Result;
}

#ifndef _UNICODE
typedef int (WINAPI * SHFileOperationWP)(LPSHFILEOPSTRUCTW lpFileOp);
#endif

void CPanel::DeleteItems(bool toRecycleBin)
{
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);
  CRecordVector<UInt32> indices;
  GetOperatedItemIndices(indices);
  if (indices.IsEmpty())
    return;
  CSelectedState state;
  SaveSelectedState(state);

  #ifndef UNDER_CE
  // WM6 / SHFileOperationW doesn't ask user! So we use internal delete
  bool useInternalDelete = false;
  if (IsFSFolder() && toRecycleBin)
  {
    #ifndef _UNICODE
    if (!g_IsNT)
    {
      CDynamicBuffer<CHAR> buffer;
      size_t size = 0;
      for (int i = 0; i < indices.Size(); i++)
      {
        const AString path = GetSystemString(GetFsPath() + GetItemRelPath(indices[i]));
        buffer.EnsureCapacity(size + path.Length() + 1);
        memmove(((CHAR *)buffer) + size, (const CHAR *)path, (path.Length() + 1) * sizeof(CHAR));
        size += path.Length() + 1;
      }
      buffer.EnsureCapacity(size + 1);
      ((CHAR *)buffer)[size]  = 0;
      SHFILEOPSTRUCTA fo;
      fo.hwnd = GetParent();
      fo.wFunc = FO_DELETE;
      fo.pFrom = (const CHAR *)buffer;
      fo.pTo = 0;
      fo.fFlags = 0;
      if (toRecycleBin)
        fo.fFlags |= FOF_ALLOWUNDO;
      // fo.fFlags |= FOF_NOCONFIRMATION;
      // fo.fFlags |= FOF_NOERRORUI;
      // fo.fFlags |= FOF_SILENT;
      // fo.fFlags |= FOF_WANTNUKEWARNING;
      fo.fAnyOperationsAborted = FALSE;
      fo.hNameMappings = 0;
      fo.lpszProgressTitle = 0;
      /* int res = */ ::SHFileOperationA(&fo);
    }
    else
    #endif
    {
      CDynamicBuffer<WCHAR> buffer;
      size_t size = 0;
      int maxLen = 0;
      for (int i = 0; i < indices.Size(); i++)
      {
        // L"\\\\?\\") doesn't work here.
        const UString path = GetFsPath() + GetItemRelPath(indices[i]);
        if (path.Length() > maxLen)
          maxLen = path.Length();
        buffer.EnsureCapacity(size + path.Length() + 1);
        memmove(((WCHAR *)buffer) + size, (const WCHAR *)path, (path.Length() + 1) * sizeof(WCHAR));
        size += path.Length() + 1;
      }
      buffer.EnsureCapacity(size + 1);
      ((WCHAR *)buffer)[size] = 0;
      if (maxLen >= MAX_PATH)
      {
        if (toRecycleBin)
        {
          MessageBoxErrorLang(IDS_ERROR_LONG_PATH_TO_RECYCLE, 0x03020218);
          return;
        }
        useInternalDelete = true;
      }
      else
      {
        SHFILEOPSTRUCTW fo;
        fo.hwnd = GetParent();
        fo.wFunc = FO_DELETE;
        fo.pFrom = (const WCHAR *)buffer;
        fo.pTo = 0;
        fo.fFlags = 0;
        if (toRecycleBin)
          fo.fFlags |= FOF_ALLOWUNDO;
        fo.fAnyOperationsAborted = FALSE;
        fo.hNameMappings = 0;
        fo.lpszProgressTitle = 0;
        int res;
        #ifdef _UNICODE
        res = ::SHFileOperationW(&fo);
        #else
        SHFileOperationWP shFileOperationW = (SHFileOperationWP)
          ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"), "SHFileOperationW");
        if (shFileOperationW == 0)
          return;
        res = shFileOperationW(&fo);
        #endif
      }
    }
    /*
    if (fo.fAnyOperationsAborted)
      MessageBoxError(result, LangString(IDS_ERROR_DELETING, 0x03020217));
    */
  }
  else
    useInternalDelete = true;
  if (useInternalDelete)
  #endif
    DeleteItemsInternal(indices);
  RefreshListCtrl(state);
}

void CPanel::MessageBoxErrorForUpdate(HRESULT errorCode, UINT resourceID, UInt32 langID)
{
  if (errorCode == E_NOINTERFACE)
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
  else
    MessageBoxError(errorCode, LangString(resourceID, langID));
}

void CPanel::DeleteItemsInternal(CRecordVector<UInt32> &indices)
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorForUpdate(E_NOINTERFACE, IDS_ERROR_DELETING, 0x03020217);
    return;
  }

  UString title;
  UString message;
  if (indices.Size() == 1)
  {
    int index = indices[0];
    const UString itemName = GetItemRelPath(index);
    if (IsItemFolder(index))
    {
      title = LangString(IDS_CONFIRM_FOLDER_DELETE, 0x03020211);
      message = MyFormatNew(IDS_WANT_TO_DELETE_FOLDER, 0x03020214, itemName);
    }
    else
    {
      title = LangString(IDS_CONFIRM_FILE_DELETE, 0x03020210);
      message = MyFormatNew(IDS_WANT_TO_DELETE_FILE, 0x03020213, itemName);
    }
  }
  else
  {
    title = LangString(IDS_CONFIRM_ITEMS_DELETE, 0x03020212);
    message = MyFormatNew(IDS_WANT_TO_DELETE_ITEMS, 0x03020215,
        NumberToString(indices.Size()));
  }
  if (::MessageBoxW(GetParent(), message, title, MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
    return;

  {
    CThreadFolderOperations op(FOLDER_TYPE_DELETE);
    op.FolderOperations = folderOperations;
    op.Indices = indices;
    op.DoOperation(*this,
        LangString(IDS_DELETING, 0x03020216),
        LangString(IDS_ERROR_DELETING, 0x03020217));
  }
  RefreshTitleAlways();
}

BOOL CPanel::OnBeginLabelEdit(LV_DISPINFOW * lpnmh)
{
  int realIndex = GetRealIndex(lpnmh->item);
  if (realIndex == kParentIndex)
    return TRUE;
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
    return TRUE;
  return FALSE;
}

BOOL CPanel::OnEndLabelEdit(LV_DISPINFOW * lpnmh)
{
  if (lpnmh->item.pszText == NULL)
    return FALSE;
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorForUpdate(E_NOINTERFACE, IDS_ERROR_RENAMING, 0x03020221);
    return FALSE;
  }
  const UString newName = lpnmh->item.pszText;
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);

  SaveSelectedState(_selectedState);

  int realIndex = GetRealIndex(lpnmh->item);
  if (realIndex == kParentIndex)
    return FALSE;
  const UString prefix = GetItemPrefix(realIndex);


  {
    CThreadFolderOperations op(FOLDER_TYPE_RENAME);
    op.FolderOperations = folderOperations;
    op.Index = realIndex;
    op.Name = newName;
    HRESULT res = op.DoOperation(*this,
        LangString(IDS_RENAMING, 0x03020220),
        LangString(IDS_ERROR_RENAMING, 0x03020221));
    if (res != S_OK)
      return FALSE;
  }

  // Can't use RefreshListCtrl here.
  // RefreshListCtrlSaveFocused();
  _selectedState.FocusedName = prefix + newName;
  _selectedState.SelectFocused = true;

  // We need clear all items to disable GetText before Reload:
  // number of items can change.
  // _listView.DeleteAllItems();
  // But seems it can still call GetText (maybe for current item)
  // so we can't delete items.

  _dontShowMode = true;

  PostMessage(kReLoadMessage);
  return TRUE;
}

void CPanel::CreateFolder()
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorForUpdate(E_NOINTERFACE, IDS_CREATE_FOLDER_ERROR, 0x03020233);
    return;
  }
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);
  CSelectedState state;
  SaveSelectedState(state);
  CComboDialog comboDialog;
  comboDialog.Title = LangString(IDS_CREATE_FOLDER, 0x03020230);
  comboDialog.Static = LangString(IDS_CREATE_FOLDER_NAME, 0x03020231);
  comboDialog.Value = LangString(IDS_CREATE_FOLDER_DEFAULT_NAME, /*0x03020232*/ (UInt32)-1);
  if (comboDialog.Create(GetParent()) == IDCANCEL)
    return;
  
  UString newName = comboDialog.Value;
  
  {
    CThreadFolderOperations op(FOLDER_TYPE_CREATE_FOLDER);
    op.FolderOperations = folderOperations;
    op.Name = newName;
    HRESULT res = op.DoOperation(*this,
        LangString(IDS_CREATE_FOLDER, 0x03020230),
        LangString(IDS_CREATE_FOLDER_ERROR, 0x03020233));
    if (res != S_OK)
      return;
  }
  int pos = newName.Find(WCHAR_PATH_SEPARATOR);
  if (pos >= 0)
    newName = newName.Left(pos);
  if (!_mySelectMode)
    state.SelectedNames.Clear();
  state.FocusedName = newName;
  state.SelectFocused = true;
  RefreshTitleAlways();
  RefreshListCtrl(state);
}

void CPanel::CreateFile()
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorForUpdate(E_NOINTERFACE, IDS_CREATE_FILE_ERROR, 0x03020243);
    return;
  }
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);
  CSelectedState state;
  SaveSelectedState(state);
  CComboDialog comboDialog;
  comboDialog.Title = LangString(IDS_CREATE_FILE, 0x03020240);
  comboDialog.Static = LangString(IDS_CREATE_FILE_NAME, 0x03020241);
  comboDialog.Value = LangString(IDS_CREATE_FILE_DEFAULT_NAME, /*0x03020242*/ (UInt32)-1);
  if (comboDialog.Create(GetParent()) == IDCANCEL)
    return;
  UString newName = comboDialog.Value;
  HRESULT result = folderOperations->CreateFile(newName, 0);
  if (result != S_OK)
  {
    MessageBoxErrorForUpdate(result, IDS_CREATE_FILE_ERROR, 0x03020243);
    return;
  }
  int pos = newName.Find(WCHAR_PATH_SEPARATOR);
  if (pos >= 0)
    newName = newName.Left(pos);
  if (!_mySelectMode)
    state.SelectedNames.Clear();
  state.FocusedName = newName;
  state.SelectFocused = true;
  RefreshListCtrl(state);
}

void CPanel::RenameFile()
{
  int index = _listView.GetFocusedItem();
  if (index >= 0)
    _listView.EditLabel(index);
}

void CPanel::ChangeComment()
{
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);
  int index = _listView.GetFocusedItem();
  if (index < 0)
    return;
  int realIndex = GetRealItemIndex(index);
  if (realIndex == kParentIndex)
    return;
  CSelectedState state;
  SaveSelectedState(state);
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    return;
  }

  UString comment;
  {
    NCOM::CPropVariant propVariant;
    if (_folder->GetProperty(realIndex, kpidComment, &propVariant) != S_OK)
      return;
    if (propVariant.vt == VT_BSTR)
      comment = propVariant.bstrVal;
    else if (propVariant.vt != VT_EMPTY)
      return;
  }
  UString name = GetItemRelPath(realIndex);
  CComboDialog comboDialog;
  comboDialog.Title = name + L" " + LangString(IDS_COMMENT, 0x03020290);
  comboDialog.Value = comment;
  comboDialog.Static = LangString(IDS_COMMENT2, 0x03020291);
  if (comboDialog.Create(GetParent()) == IDCANCEL)
    return;
  NCOM::CPropVariant propVariant = comboDialog.Value;

  HRESULT result = folderOperations->SetProperty(realIndex, kpidComment, &propVariant, NULL);
  if (result != S_OK)
  {
    if (result == E_NOINTERFACE)
      MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    else
      MessageBoxError(result, L"Set Comment Error");
  }
  RefreshListCtrl(state);
}
