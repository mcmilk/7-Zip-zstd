// PanelOperations.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Panel.h"

#include "Common/StringConvert.h"
#include "Common/DynamicBuffer.h"
#include "Windows/FileDir.h"
#include "Windows/ResourceString.h"
#include "Windows/Thread.h"
#include "Windows/COM.h"

#include "Resource/ComboDialog/ComboDialog.h"

#include "FSFolder.h"
#include "LangUtils.h"
#include "FormatUtils.h"

#include "UpdateCallback100.h"

using namespace NWindows;
using namespace NFile;

#ifndef _UNICODE
extern bool g_IsNT;
#endif

struct CThreadDelete
{
  CMyComPtr<IFolderOperations> FolderOperations;
  CRecordVector<UInt32> Indices;
  CMyComPtr<IFolderArchiveUpdateCallback> UpdateCallback;
  CUpdateCallback100Imp *UpdateCallbackSpec;
  HRESULT Result;
  
  DWORD Process()
  {
    NCOM::CComInitializer comInitializer;
    UpdateCallbackSpec->ProgressDialog.WaitCreating();
    Result = FolderOperations->Delete(&Indices.Front(), Indices.Size(), UpdateCallback);
    UpdateCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }
  
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadDelete *)param)->Process();
  }
};

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
  if (IsFSFolder())
  {
    #ifndef _UNICODE
    if (!g_IsNT)
    {
      CDynamicBuffer<CHAR> buffer;
      size_t size = 0;
      for (int i = 0; i < indices.Size(); i++)
      {
        const AString path = GetSystemString(GetFsPath() + GetItemName(indices[i]));
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
      int res = ::SHFileOperationA(&fo);
    }
    else
    #endif
    {
      CDynamicBuffer<WCHAR> buffer;
      size_t size = 0;
      for (int i = 0; i < indices.Size(); i++)
      {
        const UString path = GetFsPath() + GetItemName(indices[i]);
        buffer.EnsureCapacity(size + path.Length() + 1);
        memmove(((WCHAR *)buffer) + size, (const WCHAR *)path, (path.Length() + 1) * sizeof(WCHAR));
        size += path.Length() + 1;
      }
      buffer.EnsureCapacity(size + 1);
      ((WCHAR *)buffer)[size]  = 0;
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
    /*
    if (fo.fAnyOperationsAborted)
    {
      MessageBoxError(result, LangString(IDS_ERROR_DELETING, 0x03020217));
    }
    */
    /* 
      (!result)
      return GetLastError();
    */
  }
  else
  {

  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }

  UString title;
  UString message;
  if (indices.Size() == 1)
  {
    int index = indices[0];
    const UString itemName = GetItemName(index);
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

  CThreadDelete deleter;
  deleter.UpdateCallbackSpec = new CUpdateCallback100Imp;
  deleter.UpdateCallback = deleter.UpdateCallbackSpec;
  deleter.UpdateCallbackSpec->Init(GetParent(), false, L"");

  UString progressTitle = LangString(IDS_DELETING, 0x03020216);

  deleter.UpdateCallbackSpec->ProgressDialog.MainWindow = _mainWindow;
  deleter.UpdateCallbackSpec->ProgressDialog.MainTitle = LangString(IDS_APP_TITLE, 0x03000000);
  deleter.UpdateCallbackSpec->ProgressDialog.MainAddTitle = progressTitle + UString(L" ");

  deleter.FolderOperations = folderOperations;
  deleter.Indices = indices;

  CThread thread;
  if (!thread.Create(CThreadDelete::MyThreadFunction, &deleter))
    throw 271824;
  deleter.UpdateCallbackSpec->StartProgressDialog(progressTitle);

  HRESULT result = deleter.Result;
  if (result != S_OK)
    MessageBoxError(result, LangString(IDS_ERROR_DELETING, 0x03020217));
  }

  RefreshListCtrl(state);
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
    MessageBoxMyError(L"Renaming is not supported");
    return FALSE;
  }
  UString newName = lpnmh->item.pszText;
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);

  int realIndex = GetRealIndex(lpnmh->item);
  if (realIndex == kParentIndex)
    return FALSE;
  HRESULT result = folderOperations->Rename(realIndex, newName, 0);
  if (result != S_OK)
  {
    MessageBoxError(result, LangString(IDS_ERROR_RENAMING, 0x03020221));
    return FALSE;
  }
  // Can't use RefreshListCtrl here.
  // RefreshListCtrlSaveFocused();
  _focusedName = newName;

  // We need clear all items to disable GetText before Reload:
  // number of items can change.
  _listView.DeleteAllItems();

  PostMessage(kReLoadMessage);
  return TRUE;
}

void CPanel::CreateFolder()
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
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
  HRESULT result = folderOperations->CreateFolder(newName, 0);
  if (result != S_OK)
  {
    MessageBoxError(result, LangString(IDS_CREATE_FOLDER_ERROR, 0x03020233));
    return;
  }
  int pos = newName.Find(L'\\');
  if (pos >= 0)
    newName = newName.Left(pos);
  if (!_mySelectMode)
    state.SelectedNames.Clear();
  state.FocusedName = newName;
  state.SelectFocused = true;
  RefreshListCtrl(state);
}

void CPanel::CreateFile()
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
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
    MessageBoxError(result, LangString(IDS_CREATE_FILE_ERROR, 0x03020243));
    return;
  }
  int pos = newName.Find(L'\\');
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
    MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
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
  UString name = GetItemName(realIndex);
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
    MessageBoxError(result, L"Set Comment Error");
  }
  RefreshListCtrl(state);
}

