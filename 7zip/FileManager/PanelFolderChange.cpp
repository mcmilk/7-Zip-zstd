// PanelFolderChange.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/Wildcard.h"

#include "Windows/FileDir.h"

#include "Panel.h"
#include "Resource/ListViewDialog/ListViewDialog.h"
#include "RootFolder.h"
#include "ViewSettings.h"
#include "FSDrives.h"
#include "LangUtils.h"
#include "resource.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

void CPanel::SetToRootFolder()
{
  _folder.Release();
  _library.Free();
  CRootFolder *rootFolderSpec = new CRootFolder;
  _folder = rootFolderSpec;
  rootFolderSpec->Init();
}

HRESULT CPanel::BindToPath(const UString &fullPath)
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  CloseOpenFolders();
  CSysString sysPath = GetSystemString(fullPath);
  CFileInfo fileInfo;
  UStringVector reducedParts;
  while(!sysPath.IsEmpty())
  {
    if (FindFile(sysPath, fileInfo))
      break;
    int pos = sysPath.ReverseFind('\\');
    if (pos < 0)
      sysPath.Empty();
    else
    {
      if (reducedParts.Size() > 0 || pos < sysPath.Length() - 1)
        reducedParts.Add(GetUnicodeString(sysPath.Mid(pos + 1)));
      sysPath = sysPath.Left(pos);
    }
  }
  SetToRootFolder();
  CMyComPtr<IFolderFolder> newFolder;
  if (sysPath.IsEmpty())
  {
    if (_folder->BindToFolder(fullPath, &newFolder) == S_OK)
      _folder = newFolder;
  }
  else if (fileInfo.IsDirectory())
  {
    NName::NormalizeDirPathPrefix(sysPath);
    if (_folder->BindToFolder(GetUnicodeString(sysPath), &newFolder) == S_OK)
      _folder = newFolder;
  }
  else
  {
    CSysString dirPrefix;
    if (!NDirectory::GetOnlyDirPrefix(sysPath, dirPrefix))
      dirPrefix.Empty();
    if (_folder->BindToFolder(GetUnicodeString(dirPrefix), &newFolder) == S_OK)
    {
      _folder = newFolder;
      LoadFullPath();
      CSysString fileName;
      if (NDirectory::GetOnlyName(sysPath, fileName))
      {
        if (OpenItemAsArchive(GetUnicodeString(fileName),
            GetSystemString(_currentFolderPrefix), 
            GetSystemString(_currentFolderPrefix) + fileName) == S_OK)
        {
          for (int i = reducedParts.Size() - 1; i >= 0; i--)
          {
            CMyComPtr<IFolderFolder> newFolder;
            _folder->BindToFolder(reducedParts[i], &newFolder);
            if (!newFolder)
              break;
            _folder = newFolder;
          }
        }
      }
    }
  }
  return S_OK;
}

HRESULT CPanel::BindToPathAndRefresh(const UString &path)
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  RINOK(BindToPath(path));
  RefreshListCtrl(UString(), -1, UStringVector());
  return S_OK;
}

void CPanel::SetBookmark(int index)
{
  _appState->FastFolders.SetString(index, _currentFolderPrefix);
}

void CPanel::OpenBookmark(int index)
{
  BindToPathAndRefresh(_appState->FastFolders.GetString(index));
}

UString GetFolderPath(IFolderFolder * folder)
{
  CMyComPtr<IFolderGetPath> folderGetPath;
  if (folder->QueryInterface(&folderGetPath) == S_OK)
  {
    CMyComBSTR path;
    if (folderGetPath->GetPath(&path) == S_OK)
      return (const wchar_t *)path;
  }
  return UString();
}

void CPanel::LoadFullPath()
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

void CPanel::LoadFullPathAndShow()
{ 
  LoadFullPath();
  _appState->FolderHistrory.AddString(_currentFolderPrefix);

  // _headerComboBox.SendMessage(CB_RESETCONTENT, 0, 0);
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

  if (info->iWhy == CBENF_RETURN)
  {
    if (BindToPathAndRefresh(GetUnicodeString(info->szText)) != S_OK)
    {
      // MessageBeep((UINT)-1);
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
    CRootFolder *rootFolderSpec = new CRootFolder;
    CMyComPtr<IFolderFolder> rootFolder = rootFolderSpec;
    rootFolderSpec->Init();
    CMyComPtr<IFolderFolder> newFolder;
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
    case CBEN_BEGINEDIT:
    {
      _lastFocusedIsList = false;
      _panelCallback->PanelWasFocused();
    }
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
  listViewDialog.DeleteIsAllowed = true;
  // listViewDialog.m_Value = TEXT("*");
  listViewDialog.Title = LangLoadStringW(IDS_FOLDERS_HISTORY, 0x03020260);
  UStringVector strings;
  _appState->FolderHistrory.GetList(strings);
  int i;
  for(i = 0; i < strings.Size(); i++)
    listViewDialog.Strings.Add(GetSystemString(strings[i]));
  if (listViewDialog.Create(GetParent()) == IDCANCEL)
    return;
  UString selectString;
  if (listViewDialog.StringsWereChanged)
  {
    _appState->FolderHistrory.RemoveAll();
    for (i = listViewDialog.Strings.Size() - 1; i >= 0; i--)
      _appState->FolderHistrory.AddString(GetUnicodeString(listViewDialog.Strings[i]));
    if (listViewDialog.FocusedItemIndex >= 0)
      selectString = GetUnicodeString(listViewDialog.Strings[listViewDialog.FocusedItemIndex]);
  }
  else
  {
    if (listViewDialog.FocusedItemIndex >= 0)
      selectString = strings[listViewDialog.FocusedItemIndex];
  }
  if (listViewDialog.FocusedItemIndex >= 0)
    BindToPathAndRefresh(selectString);
}

void CPanel::OpenParentFolder()
{
  LoadFullPath(); // Maybe we don't need it ??
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

  CDisableTimerProcessing disableTimerProcessing1(*this);
  CMyComPtr<IFolderFolder> newFolder;
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
      _folder.Release();
      _library.Free();
      CFolderLink &link = _parentFolders.Back();
      _folder = link.ParentFolder;
      _library.Attach(link.Library.Detach());
      focucedName = link.ItemName;
      if (_parentFolders.Size () > 1)
        OpenParentArchiveFolder();
      _parentFolders.DeleteBack();
    }
  }

  UStringVector selectedItems;
  /*
  if (!focucedName.IsEmpty())
    selectedItems.Add(focucedName);
  */
  LoadFullPath();
  ::SetCurrentDirectory(::GetSystemString(_currentFolderPrefix));
  RefreshListCtrl(focucedName, -1, selectedItems);
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
  RefreshStatusBar();
}

void CPanel::CloseOpenFolders()
{
  while(_parentFolders.Size() > 0)
  {
    _folder.Release();
    _library.Free();
    _folder = _parentFolders.Back().ParentFolder;
    _library.Attach(_parentFolders.Back().Library.Detach());
    if (_parentFolders.Size () > 1)
      OpenParentArchiveFolder();
    _parentFolders.DeleteBack();
  }
  _folder.Release();
  _library.Free();
}

void CPanel::OpenRootFolder()
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  _parentFolders.Clear();
  SetToRootFolder();
  RefreshListCtrl(UString(), 0, UStringVector());
  // ::SetCurrentDirectory(::GetSystemString(_currentFolderPrefix));
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
  CFSDrives *fsFolderSpec = new CFSDrives;
  _folder = fsFolderSpec;
  fsFolderSpec->Init();
  RefreshListCtrl();
}

void CPanel::OpenFolder(int index)
{
  if (index == -1)
  {
    OpenParentFolder();
    return;
  }
  CMyComPtr<IFolderFolder> newFolder;
  _folder->BindToFolder(index, &newFolder);
  if (!newFolder)
    return;
  _folder = newFolder;
  LoadFullPath();
  ::SetCurrentDirectory(::GetSystemString(_currentFolderPrefix));
  RefreshListCtrl();
  UINT state = LVIS_SELECTED;
  _listView.SetItemState(_listView.GetFocusedItem(), state, state);
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
}
