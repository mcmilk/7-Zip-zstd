// PanelFolderChange.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/StringConvert.h"
#include "Common/Wildcard.h"

#include "Panel.h"

#include "Resource/ListViewDialog/ListViewDialog.h"

#include "RootFolder.h"
#include "ViewSettings.h"

#include "FSDrives.h"
#include "LangUtils.h"

using namespace NWindows;

void CPanel::FastFolderInsert(int index)
{
  _appState->FastFolders.SetString(index, _currentFolderPrefix);
}

void CPanel::FastFolderSelect(int index)
{
  BindToFolder(_appState->FastFolders.GetString(index));
}

UString GetFolderPath(IFolderFolder * folder)
{
  CComPtr<IFolderGetPath> folderGetPath;
  if (folder->QueryInterface(&folderGetPath) == S_OK)
  {
    CComBSTR path;
    if (folderGetPath->GetPath(&path) == S_OK)
      return (const wchar_t *)path;
  }
  return UString();
}

void CPanel::LoadCurrentPath()
{
  _currentFolderPrefix.Empty();
  for (int i = 0; i < _parentFolders.Size(); i++)
  {
    const CFolderLink &folderLink = _parentFolders[i];
    _currentFolderPrefix += GetFolderPath(folderLink.ParentFolder);
    _currentFolderPrefix += folderLink.ItemName;
    _currentFolderPrefix += L'\\';
  }
  if (_folder)
    _currentFolderPrefix += GetFolderPath(_folder);
}

void CPanel::SetCurrentPathText()
{ 
  LoadCurrentPath();
  _appState->FolderHistrory.AddString(_currentFolderPrefix);

  _headerComboBox.SendMessage(CB_RESETCONTENT, 0, 0);
  _headerComboBox.SetText(GetSystemString(_currentFolderPrefix)); 

  /*
  for (int i = 0; i < g_Folders.m_Strings.Size(); i++)
  {
    CSysString string = GetSystemString(g_Folders.m_Strings[i]);
    COMBOBOXEXITEM item;
    item.mask = CBEIF_TEXT;
    item.iItem = i;
    item.pszText = (LPTSTR)(LPCTSTR)string;
    _headerComboBox.InsertItem(&item);
  }
  */
}

HRESULT CPanel::BindToFolder(const UString &path)
{
  while(_parentFolders.Size() > 0)
  {
    int numLevels = _parentFolders.Size();
    OpenParentFolder();
    if (numLevels == _parentFolders.Size())
      return E_FAIL;
  }

  CComObjectNoLock<CRootFolder> *rootFolderSpec = new CComObjectNoLock<CRootFolder>;
  CComPtr<IFolderFolder> rootFolder = rootFolderSpec;
  rootFolderSpec->Init();
  CComPtr<IFolderFolder> newFolder;
  RETURN_IF_NOT_S_OK(rootFolder->BindToFolder(path, &newFolder));
  _folder = newFolder;
  SetCurrentPathText();
  RefreshListCtrl(UString(), -1, UStringVector());
  return S_OK;
}

bool CPanel::OnNotifyComboBoxEndEdit(PNMCBEENDEDIT info, LRESULT &result)
{
  if (info->iWhy == CBENF_ESCAPE)
  {
    _headerComboBox.SetText(GetSystemString(_currentFolderPrefix)); 
    PostMessage(kSetFocusToListView);
    result = FALSE;
    return true;
  }
  if (info->iWhy == CBENF_DROPDOWN)
  {
    result = FALSE;
    return true;
  }

  /*
  if (info->iWhy == CBENF_DROPDOWN && info->fChanged == FALSE)
  {
    result = FALSE;
    return true;
  }
  */


  if (info->iWhy == CBENF_RETURN)
  {
    if (BindToFolder(GetUnicodeString(info->szText)) != S_OK)
    {
      MessageBeep(-1);
      result = TRUE;
      return true;
    }
    result = FALSE;
    PostMessage(kSetFocusToListView);
    return true;
  }
  return false;
}

void CPanel::OnComboBoxCommand(UINT code, LPARAM &param)
{
  /*
  if (code == CBN_SELENDOK)
  {
    CSysString path;
    if (!_headerComboBox.GetText(path))
      return;
    CComObjectNoLock<CRootFolder> *rootFolderSpec = new CComObjectNoLock<CRootFolder>;
    CComPtr<IFolderFolder> rootFolder = rootFolderSpec;
    rootFolderSpec->Init();
    CComPtr<IFolderFolder> newFolder;
    if (rootFolder->BindToFolder(GetUnicodeString(path), 
      &newFolder) != S_OK)
    {
      return;
    }
    _folder = newFolder;
    SetCurrentPathText();
    RefreshListCtrl(UString(), -1, UStringVector());
    PostMessage(kSetFocusToListView);
  }
  */
}

bool CPanel::OnNotifyComboBox(LPNMHDR header, LRESULT &result)
{
  switch(header->code)
  {
    case CBEN_ENDEDIT:
    {
      return OnNotifyComboBoxEndEdit((PNMCBEENDEDIT)header, result);
    }
  }
  return false;
}


void CPanel::FoldersHistory()
{
  CListViewDialog listViewDialog;
  // listViewDialog.m_Value = TEXT("*");
  listViewDialog.Title = LangLoadString(IDS_FOLDERS_HISTORY, 0x03020260);
  UStringVector strings;
  _appState->FolderHistrory.GetList(strings);
  for(int i = 0; i < strings.Size(); i++)
    listViewDialog.Strings.Add(GetSystemString(strings[i]));
  if (listViewDialog.Create(GetParent()) == IDCANCEL)
    return;
  if (listViewDialog.ItemIndex >= 0)
    BindToFolder(strings[listViewDialog.ItemIndex]);
}
 
void CPanel::OpenParentFolder()
{
  LoadCurrentPath();
  UString focucedName;
  if (!_currentFolderPrefix.IsEmpty())
  {
    UString string = _currentFolderPrefix;
    string.Delete(string.Length() - 1);
    int pos = string.ReverseFind(L'\\');
    if (pos < 0)
      pos = 0;
    else
      pos++;
    focucedName = GetUnicodeString(string.Mid(pos));
  }

  CComPtr<IFolderFolder> newFolder;
  _folder->BindToParentFolder(&newFolder);
  if (newFolder)
    _folder = newFolder;
  else
  {
    if (_parentFolders.IsEmpty())
    {
      SetToRootFolder();
      if (focucedName.IsEmpty())
        focucedName = GetItemName(0);
    }
    else
    {
      _folder = _parentFolders.Back().ParentFolder;
      if (_parentFolders.Size () > 1)
        OpenParentArchiveFolder();
      focucedName = _parentFolders.Back().ItemName;
      _parentFolders.DeleteBack();
    }
  }

  SetCurrentPathText();

  UStringVector selectedItems;
  /*
  if (!focucedName.IsEmpty())
    selectedItems.Add(focucedName);
  */
  RefreshListCtrl(focucedName, -1, selectedItems);
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
}

void CPanel::CloseOpenFolders()
{
  while(_parentFolders.Size() > 1)
  {
    _folder = _parentFolders.Back().ParentFolder;
    OpenParentArchiveFolder();
    _parentFolders.DeleteBack();
  }
}

void CPanel::OpenRootFolder()
{
  _parentFolders.Clear();
  SetToRootFolder();
  RefreshListCtrl(UString(), 0, UStringVector());
  /*
  BeforeChangeFolder();
  _currentFolderPrefix.Empty();
  AfterChangeFolder();
  SetCurrentPathText();
  RefreshListCtrl(CSysString(), 0, CSysStringVector());
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
  */
}

void CPanel::OpenDrivesFolder()
{
  CComObjectNoLock<CFSDrives> *fsFolderSpec = new CComObjectNoLock<CFSDrives>;
  _folder = fsFolderSpec;
  fsFolderSpec->Init();
  RefreshListCtrl();
}

void CPanel::OpenFolder(int index)
{
  CComPtr<IFolderFolder> newFolder;
  _folder->BindToFolder(index, &newFolder);
  if (!newFolder)
    return;
  _folder = newFolder;
  SetCurrentPathText();
  RefreshListCtrl();
  UINT state = LVIS_SELECTED;
  _listView.SetItemState(_listView.GetFocusedItem(), state, state);
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
}
