// PanelOperations.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Panel.h"

#include "Common/StringConvert.h"
#include "Windows/FileDir.h"
#include "Windows/ResourceString.h"
#include "Windows/Thread.h"
#include "Windows/COM.h"

#include "Resource/ComboDialog/ComboDialog.h"

#include "FSFolder.h"
#include "FormatUtils.h"

#include "UpdateCallback100.h"

using namespace NWindows;
using namespace NFile;


struct CThreadDelete
{
  CComPtr<IFolderOperations> FolderOperations;
  CRecordVector<UINT32> Indices;
  CComPtr<IFolderArchiveUpdateCallback> UpdateCallback;
  CComObjectNoLock<CUpdateCallback100Imp> *UpdateCallbackSpec;
  HRESULT Result;
  
  DWORD Process()
  {
    NCOM::CComInitializer comInitializer;
    UpdateCallbackSpec->ProgressDialog.WaitCreating();
    Result = FolderOperations->Delete(&Indices.Front(), 
        Indices.Size(), UpdateCallback);
    UpdateCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }
  
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadDelete *)param)->Process();
  }
};


void CPanel::DeleteItems()
{
  CComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(&folderOperations) != S_OK)
  {
    MessageBox(LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }

  CRecordVector<UINT32> indices;
  GetOperatedItemIndexes(indices);
  if (indices.IsEmpty())
    return;
  UString title;
  UString message;
  if (indices.Size() == 1)
  {
    int index = indices[0];
    const UString itemName = GetItemName(index);
    if (IsItemFolder(index))
    {
      title = LangLoadStringW(IDS_CONFIRM_FOLDER_DELETE, 0x03020211);
      message = MyFormatNew(IDS_WANT_TO_DELETE_FOLDER, 0x03020214, itemName);
    }
    else
    {
      title = LangLoadStringW(IDS_CONFIRM_FILE_DELETE, 0x03020210);
      message = MyFormatNew(IDS_WANT_TO_DELETE_FILE, 0x03020213, itemName);
    }
  }
  else
  {
    title = LangLoadStringW(IDS_CONFIRM_ITEMS_DELETE, 0x03020212);
    message = MyFormatNew(IDS_WANT_TO_DELETE_ITEMS, 0x03020215, 
        NumberToStringW(indices.Size()));
  }
  if (::MessageBoxW(GetParent(), message, title, MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
    return;


  CThreadDelete deleter;
  deleter.UpdateCallbackSpec = new CComObjectNoLock<CUpdateCallback100Imp>;
  deleter.UpdateCallback = deleter.UpdateCallbackSpec;
  deleter.UpdateCallbackSpec->Init(GetParent(), false, L"");

  CSysString progressTitle = LangLoadString(IDS_DELETING, 0x03020216);

  deleter.UpdateCallbackSpec->ProgressDialog.MainWindow = _mainWindow;
  deleter.UpdateCallbackSpec->ProgressDialog.MainTitle = LangLoadString(IDS_APP_TITLE, 0x03000000);
  deleter.UpdateCallbackSpec->ProgressDialog.MainAddTitle = progressTitle + CSysString(TEXT(" "));

  deleter.FolderOperations = folderOperations;
  deleter.Indices = indices;

  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);

  CThread thread;
  if (!thread.Create(CThreadDelete::MyThreadFunction, &deleter))
    throw 271824;
  deleter.UpdateCallbackSpec->StartProgressDialog(progressTitle);

  HRESULT result = deleter.Result;
  if (result != S_OK)
    MessageBoxError(result, LangLoadStringW(IDS_ERROR_DELETING, 0x03020217));

  RefreshListCtrlSaveFocused();
}

BOOL CPanel::OnBeginLabelEdit(LV_DISPINFO * lpnmh)
{
  CComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(&folderOperations) != S_OK)
    return TRUE;
  return FALSE;
}

BOOL CPanel::OnEndLabelEdit(LV_DISPINFO * lpnmh)
{
  if (lpnmh->item.pszText == NULL)
    return FALSE;
  CComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(&folderOperations) != S_OK)
  {
    MessageBoxMyError(L"Renaming is not supported");
    return FALSE;
  }
  UString newName = GetUnicodeString(lpnmh->item.pszText);
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);
  HRESULT result = folderOperations->Rename(GetRealIndex(lpnmh->item), newName, 0);
  if (result != S_OK)
  {
    MessageBoxError(result, LangLoadStringW(IDS_ERROR_RENAMING, 0x03020221));
    return FALSE;
  }
  // Can't use RefreshListCtrl here.
  // UStringVector selectedItems;
  // selectedItems.Add(newName);
  // RefreshListCtrl(newName, -1, selectedItems);
  // RefreshListCtrl();
  PostMessage(kReLoadMessage);
  return TRUE;
}

void CPanel::CreateFolder()
{
  CComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(&folderOperations) != S_OK)
  {
    MessageBox(LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }
  CComboDialog comboDialog;
  comboDialog.Title = LangLoadString(IDS_CREATE_FOLDER, 0x03020230);
  comboDialog.Static = LangLoadString(IDS_CREATE_FOLDER_NAME, 0x03020231);
  comboDialog.Value = LangLoadString(IDS_CREATE_FOLDER_DEFAULT_NAME, /*0x03020232*/ -1);
  if (comboDialog.Create(GetParent()) == IDCANCEL)
    return;
  UString newName = GetUnicodeString(comboDialog.Value);
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);
  HRESULT result = folderOperations->CreateFolder(newName, 0);
  if (result != S_OK)
  {
    MessageBoxError(result, LangLoadStringW(IDS_CREATE_FOLDER_ERROR, 0x03020233));
    return;
  }
  UStringVector selectedNames;
  GetSelectedNames(selectedNames);
  int pos = newName.Find(TEXT('\\'));
  if (pos >= 0)
    newName = newName.Left(pos);
  SetFocus();
  RefreshListCtrl(newName, _listView.GetFocusedItem(), selectedNames);
}

void CPanel::CreateFile()
{
  CComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(&folderOperations) != S_OK)
  {
    MessageBox(LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }
  CComboDialog comboDialog;
  comboDialog.Title = LangLoadString(IDS_CREATE_FILE, 0x03020240);
  comboDialog.Static = LangLoadString(IDS_CREATE_FILE_NAME, 0x03020241);
  comboDialog.Value = LangLoadString(IDS_CREATE_FILE_DEFAULT_NAME, /*0x03020242*/ -1);
  if (comboDialog.Create(GetParent()) == IDCANCEL)
    return;
  UString newName = GetUnicodeString(comboDialog.Value);
  CPanel::CDisableTimerProcessing disableTimerProcessing2(*this);
  HRESULT result = folderOperations->CreateFile(newName, 0);
  if (result != S_OK)
  {
    MessageBoxError(result, LangLoadStringW(IDS_CREATE_FILE_ERROR, 0x03020243));
    return;
  }
  UStringVector selectedNames;
  GetSelectedNames(selectedNames);
  int pos = newName.Find(TEXT('\\'));
  if (pos >= 0)
    newName = newName.Left(pos);
  RefreshListCtrl(newName, _listView.GetFocusedItem(),
      selectedNames);
}

void CPanel::RenameFile()
{
  int index = _listView.GetFocusedItem();
  if (index >= 0)
    _listView.EditLabel(index);
}

void CPanel::ChangeComment()
{
  int index = _listView.GetFocusedItem();
  if (index < 0)
    return;
  CComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(&folderOperations) != S_OK)
  {
    MessageBox(LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }

  int realIndex = GetRealItemIndex(index);
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
  comboDialog.Title = GetSystemString(name) + TEXT(" comment");
  comboDialog.Value = GetSystemString(comment);
  comboDialog.Static = TEXT("&Comment:");
  if (comboDialog.Create(GetParent()) == IDCANCEL)
    return;
  NCOM::CPropVariant propVariant = GetUnicodeString(comboDialog.Value);
  HRESULT result = folderOperations->SetProperty(realIndex, kpidComment, &propVariant, NULL);
  if (result != S_OK)
  {
    MessageBoxError(result, L"Set Comment Error");
  }
  RefreshListCtrlSaveFocused();
}

