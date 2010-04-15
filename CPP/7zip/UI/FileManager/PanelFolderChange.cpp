// PanelFolderChange.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/Wildcard.h"

#include "Windows/PropVariant.h"

#include "../../PropID.h"

#ifdef UNDER_CE
#include "FSFolder.h"
#else
#include "FSDrives.h"
#endif
#include "LangUtils.h"
#include "ListViewDialog.h"
#include "Panel.h"
#include "RootFolder.h"
#include "ViewSettings.h"

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

HRESULT CPanel::BindToPath(const UString &fullPath, const UString &arcFormat, bool &archiveIsOpened, bool &encrypted)
{
  archiveIsOpened = false;
  encrypted = false;
  CDisableTimerProcessing disableTimerProcessing1(*this);

  if (_parentFolders.Size() > 0)
  {
    const UString &virtPath = _parentFolders.Back().VirtualPath;
    if (fullPath.Left(virtPath.Length()) == virtPath)
    {
      for (;;)
      {
        CMyComPtr<IFolderFolder> newFolder;
        HRESULT res = _folder->BindToParentFolder(&newFolder);
        if (!newFolder || res != S_OK)
          break;
        _folder = newFolder;
      }
      UStringVector parts;
      SplitPathToParts(fullPath.Mid(virtPath.Length()), parts);
      for (int i = 0; i < parts.Size(); i++)
      {
        const UString &s = parts[i];
        if ((i == 0 || i == parts.Size() - 1) && s.IsEmpty())
          continue;
        CMyComPtr<IFolderFolder> newFolder;
        HRESULT res = _folder->BindToFolder(s, &newFolder);
        if (!newFolder || res != S_OK)
          break;
        _folder = newFolder;
      }
      return S_OK;
    }
  }

  CloseOpenFolders();
  UString sysPath = fullPath;
  CFileInfoW fileInfo;
  UStringVector reducedParts;
  while (!sysPath.IsEmpty())
  {
    if (fileInfo.Find(sysPath))
      break;
    int pos = sysPath.ReverseFind(WCHAR_PATH_SEPARATOR);
    if (pos < 0)
      sysPath.Empty();
    else
    {
      if (reducedParts.Size() > 0 || pos < sysPath.Length() - 1)
        reducedParts.Add(sysPath.Mid(pos + 1));
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
  else if (fileInfo.IsDir())
  {
    NName::NormalizeDirPathPrefix(sysPath);
    if (_folder->BindToFolder(sysPath, &newFolder) == S_OK)
      _folder = newFolder;
  }
  else
  {
    UString dirPrefix;
    if (!NDirectory::GetOnlyDirPrefix(sysPath, dirPrefix))
      dirPrefix.Empty();
    if (_folder->BindToFolder(dirPrefix, &newFolder) == S_OK)
    {
      _folder = newFolder;
      LoadFullPath();
      UString fileName;
      if (NDirectory::GetOnlyName(sysPath, fileName))
      {
        HRESULT res = OpenItemAsArchive(fileName, arcFormat, encrypted);
        if (res != S_FALSE)
        {
          RINOK(res);
        }
        /*
        if (res == E_ABORT)
          return res;
        */
        if (res == S_OK)
        {
          archiveIsOpened = true;
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
  bool archiveIsOpened, encrypted;
  RINOK(BindToPath(path, UString(), archiveIsOpened, encrypted));
  RefreshListCtrl(UString(), -1, true, UStringVector());
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

UString GetFolderPath(IFolderFolder *folder)
{
  NCOM::CPropVariant prop;
  if (folder->GetFolderProperty(kpidPath, &prop) == S_OK)
    if (prop.vt == VT_BSTR)
      return (wchar_t *)prop.bstrVal;
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
    _currentFolderPrefix += WCHAR_PATH_SEPARATOR;
  }
  if (_folder)
    _currentFolderPrefix += GetFolderPath(_folder);
}

static int GetRealIconIndex(LPCWSTR path, DWORD attributes)
{
  int index = -1;
  if (GetRealIconIndex(path, attributes, index) != 0)
    return index;
  return -1;
}

void CPanel::LoadFullPathAndShow()
{
  LoadFullPath();
  _appState->FolderHistory.AddString(_currentFolderPrefix);

  _headerComboBox.SetText(_currentFolderPrefix);

  #ifndef UNDER_CE
  COMBOBOXEXITEM item;
  item.mask = 0;

  UString path = _currentFolderPrefix;
  if (path.Length() >
      #ifdef _WIN32
      3
      #else
      1
      #endif
      && path[path.Length() - 1] == WCHAR_PATH_SEPARATOR)
    path.Delete(path.Length() - 1);

  CFileInfoW info;
  DWORD attrib = FILE_ATTRIBUTE_DIRECTORY;
  if (info.Find(path))
    attrib = info.Attrib;
  
  item.iImage = GetRealIconIndex(path, attrib);

  if (item.iImage >= 0)
  {
    item.iSelectedImage = item.iImage;
    item.mask |= (CBEIF_IMAGE | CBEIF_SELECTEDIMAGE);
  }
  item.iItem = -1;
  _headerComboBox.SetItem(&item);
  #endif

  RefreshTitle();
}

#ifndef UNDER_CE
LRESULT CPanel::OnNotifyComboBoxEnter(const UString &s)
{
  if (BindToPathAndRefresh(GetUnicodeString(s)) == S_OK)
  {
    PostMessage(kSetFocusToListView);
    return TRUE;
  }
  return FALSE;
}

bool CPanel::OnNotifyComboBoxEndEdit(PNMCBEENDEDITW info, LRESULT &result)
{
  if (info->iWhy == CBENF_ESCAPE)
  {
    _headerComboBox.SetText(_currentFolderPrefix);
    PostMessage(kSetFocusToListView);
    result = FALSE;
    return true;
  }

  /*
  if (info->iWhy == CBENF_DROPDOWN)
  {
    result = FALSE;
    return true;
  }
  */

  if (info->iWhy == CBENF_RETURN)
  {
    // When we use Edit control and press Enter.
    UString s;
    _headerComboBox.GetText(s);
    result = OnNotifyComboBoxEnter(s);
    return true;
  }
  return false;
}
#endif

#ifndef _UNICODE
bool CPanel::OnNotifyComboBoxEndEdit(PNMCBEENDEDIT info, LRESULT &result)
{
  if (info->iWhy == CBENF_ESCAPE)
  {
    _headerComboBox.SetText(_currentFolderPrefix);
    PostMessage(kSetFocusToListView);
    result = FALSE;
    return true;
  }
  /*
  if (info->iWhy == CBENF_DROPDOWN)
  {
    result = FALSE;
    return true;
  }
  */

  if (info->iWhy == CBENF_RETURN)
  {
    UString s;
    _headerComboBox.GetText(s);
    // GetUnicodeString(info->szText)
    result = OnNotifyComboBoxEnter(s);
    return true;
  }
  return false;
}
#endif

void CPanel::AddComboBoxItem(const UString &name, int iconIndex, int indent, bool addToList)
{
  #ifdef UNDER_CE

  UString s;
  iconIndex = iconIndex;
  for (int i = 0; i < indent; i++)
    s += L"  ";
  _headerComboBox.AddString(s + name);
  
  #else
  
  COMBOBOXEXITEMW item;
  item.mask = CBEIF_TEXT | CBEIF_INDENT;
  item.iSelectedImage = item.iImage = iconIndex;
  if (iconIndex >= 0)
    item.mask |= (CBEIF_IMAGE | CBEIF_SELECTEDIMAGE);
  item.iItem = -1;
  item.iIndent = indent;
  item.pszText = (LPWSTR)(LPCWSTR)name;
  _headerComboBox.InsertItem(&item);
  
  #endif

  if (addToList)
    ComboBoxPaths.Add(name);
}

extern UString RootFolder_GetName_Computer(int &iconIndex);
extern UString RootFolder_GetName_Network(int &iconIndex);
extern UString RootFolder_GetName_Documents(int &iconIndex);

bool CPanel::OnComboBoxCommand(UINT code, LPARAM /* param */, LRESULT &result)
{
  result = FALSE;
  switch(code)
  {
    case CBN_DROPDOWN:
    {
      ComboBoxPaths.Clear();
      _headerComboBox.ResetContent();
      
      int i;
      UStringVector pathParts;
      
      SplitPathToParts(_currentFolderPrefix, pathParts);
      UString sumPass;
      if (!pathParts.IsEmpty())
        pathParts.DeleteBack();
      for (i = 0; i < pathParts.Size(); i++)
      {
        UString name = pathParts[i];
        sumPass += name;
        sumPass += WCHAR_PATH_SEPARATOR;
        CFileInfoW info;
        DWORD attrib = FILE_ATTRIBUTE_DIRECTORY;
        if (info.Find(sumPass))
          attrib = info.Attrib;
        AddComboBoxItem(name.IsEmpty() ? L"\\" : name, GetRealIconIndex(sumPass, attrib), i, false);
        ComboBoxPaths.Add(sumPass);
      }

      #ifndef UNDER_CE

      int iconIndex;
      UString name;
      name = RootFolder_GetName_Documents(iconIndex);
      AddComboBoxItem(name, iconIndex, 0, true);

      name = RootFolder_GetName_Computer(iconIndex);
      AddComboBoxItem(name, iconIndex, 0, true);
        
      UStringVector driveStrings;
      MyGetLogicalDriveStrings(driveStrings);
      for (i = 0; i < driveStrings.Size(); i++)
      {
        UString s = driveStrings[i];
        ComboBoxPaths.Add(s);
        int iconIndex = GetRealIconIndex(s, 0);
        if (s.Length() > 0 && s[s.Length() - 1] == WCHAR_PATH_SEPARATOR)
          s.Delete(s.Length() - 1);
        AddComboBoxItem(s, iconIndex, 1, false);
      }

      name = RootFolder_GetName_Network(iconIndex);
      AddComboBoxItem(name, iconIndex, 0, true);

      #endif
    
      return false;
    }

    case CBN_SELENDOK:
    {
      code = code;
      int index = _headerComboBox.GetCurSel();
      if (index >= 0)
      {
        UString pass = ComboBoxPaths[index];
        _headerComboBox.SetCurSel(-1);
        // _headerComboBox.SetText(pass); // it's fix for seclecting by mouse.
        if (BindToPathAndRefresh(pass) == S_OK)
        {
          PostMessage(kSetFocusToListView);
          #ifdef UNDER_CE
          PostMessage(kRefreshHeaderComboBox);
          #endif

          return true;
        }
      }
      return false;
    }
    /*
    case CBN_CLOSEUP:
    {
      LoadFullPathAndShow();
      true;

    }
    case CBN_SELCHANGE:
    {
      // LoadFullPathAndShow();
      return true;
    }
    */
  }
  return false;
}

bool CPanel::OnNotifyComboBox(LPNMHDR header, LRESULT &result)
{
  #ifndef UNDER_CE
  switch(header->code)
  {
    case CBEN_BEGINEDIT:
    {
      _lastFocusedIsList = false;
      _panelCallback->PanelWasFocused();
      break;
    }
    #ifndef _UNICODE
    case CBEN_ENDEDIT:
    {
      return OnNotifyComboBoxEndEdit((PNMCBEENDEDIT)header, result);
    }
    #endif
    case CBEN_ENDEDITW:
    {
      return OnNotifyComboBoxEndEdit((PNMCBEENDEDITW)header, result);
    }
  }
  #endif
  return false;
}


void CPanel::FoldersHistory()
{
  CListViewDialog listViewDialog;
  listViewDialog.DeleteIsAllowed = true;
  listViewDialog.Title = LangString(IDS_FOLDERS_HISTORY, 0x03020260);
  _appState->FolderHistory.GetList(listViewDialog.Strings);
  if (listViewDialog.Create(GetParent()) == IDCANCEL)
    return;
  UString selectString;
  if (listViewDialog.StringsWereChanged)
  {
    _appState->FolderHistory.RemoveAll();
    for (int i = listViewDialog.Strings.Size() - 1; i >= 0; i--)
      _appState->FolderHistory.AddString(listViewDialog.Strings[i]);
    if (listViewDialog.FocusedItemIndex >= 0)
      selectString = listViewDialog.Strings[listViewDialog.FocusedItemIndex];
  }
  else
  {
    if (listViewDialog.FocusedItemIndex >= 0)
      selectString = listViewDialog.Strings[listViewDialog.FocusedItemIndex];
  }
  if (listViewDialog.FocusedItemIndex >= 0)
    BindToPathAndRefresh(selectString);
}

void CPanel::OpenParentFolder()
{
  LoadFullPath(); // Maybe we don't need it ??
  UString focucedName;
  if (!_currentFolderPrefix.IsEmpty() &&
      _currentFolderPrefix.Back() == WCHAR_PATH_SEPARATOR)
  {
    focucedName = _currentFolderPrefix;
    focucedName.DeleteBack();
    if (focucedName != L"\\\\.")
    {
      int pos = focucedName.ReverseFind(WCHAR_PATH_SEPARATOR);
      if (pos >= 0)
        focucedName = focucedName.Mid(pos + 1);
    }
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
      if (_parentFolders.Size() > 1)
        OpenParentArchiveFolder();
      _parentFolders.DeleteBack();
      if (_parentFolders.IsEmpty())
        _flatMode = _flatModeForDisk;
    }
  }

  UStringVector selectedItems;
  /*
  if (!focucedName.IsEmpty())
    selectedItems.Add(focucedName);
  */
  LoadFullPath();
  // ::SetCurrentDirectory(::_currentFolderPrefix);
  RefreshListCtrl(focucedName, -1, true, selectedItems);
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
  RefreshStatusBar();
}

void CPanel::CloseOpenFolders()
{
  while (_parentFolders.Size() > 0)
  {
    _folder.Release();
    _library.Free();
    _folder = _parentFolders.Back().ParentFolder;
    _library.Attach(_parentFolders.Back().Library.Detach());
    if (_parentFolders.Size() > 1)
      OpenParentArchiveFolder();
    _parentFolders.DeleteBack();
  }
  _flatMode = _flatModeForDisk;
  _folder.Release();
  _library.Free();
}

void CPanel::OpenRootFolder()
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  _parentFolders.Clear();
  SetToRootFolder();
  RefreshListCtrl(UString(), -1, true, UStringVector());
  // ::SetCurrentDirectory(::_currentFolderPrefix);
  /*
  BeforeChangeFolder();
  _currentFolderPrefix.Empty();
  AfterChangeFolder();
  SetCurrentPathText();
  RefreshListCtrl(UString(), 0, UStringVector());
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
  */
}

void CPanel::OpenDrivesFolder()
{
  CloseOpenFolders();
  #ifdef UNDER_CE
  NFsFolder::CFSFolder *folderSpec = new NFsFolder::CFSFolder;
  _folder = folderSpec;
  folderSpec->InitToRoot();
  #else
  CFSDrives *folderSpec = new CFSDrives;
  _folder = folderSpec;
  folderSpec->Init();
  #endif
  RefreshListCtrl();
}

void CPanel::OpenFolder(int index)
{
  if (index == kParentIndex)
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
  // ::SetCurrentDirectory(::_currentFolderPrefix);
  RefreshListCtrl();
  UINT state = LVIS_SELECTED;
  _listView.SetItemState(_listView.GetFocusedItem(), state, state);
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
}
