// PanelItems.cpp

#include "StdAfx.h"

#include "Common/String.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/Menu.h"

#include "Interface/PropID.h"

#include "Panel.h"

#include "RootFolder.h"

#include "PropertyName.h"

using namespace NWindows;

static int GetColumnAlign(PROPID propID, VARTYPE varType)
{
  switch(propID)
  {
    case kpidCreationTime:
    case kpidLastAccessTime:
    case kpidLastWriteTime:
      return LVCFMT_LEFT;
  }
  switch(varType)
  {
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_I4:
    case VT_INT:
    case VT_UI4:
    case VT_UINT:
    case VT_I8:
    case VT_UI8:
    case VT_BOOL:    
      return LVCFMT_RIGHT;
    
    case VT_EMPTY:
    case VT_I1:
    case VT_FILETIME:
    case VT_BSTR:
      return LVCFMT_LEFT;
    
    default:
      return LVCFMT_CENTER;
  }
}

void CPanel::InitColumns()
{
  if (_needSaveInfo)
    SaveListViewInfo();

  _listView.DeleteAllItems();
  _selectedStatusVector.Clear();

  ReadListViewInfo();


  PROPID sortID;
  /*
  if (_listViewInfo.SortIndex >= 0)
    sortID = _listViewInfo.Columns[_listViewInfo.SortIndex].PropID;
  */
  sortID  = _listViewInfo.SortID;

  _ascending = _listViewInfo.Ascending;

  _properties.Clear();

  CComPtr<IEnumProperties> enumProperties;
  CComPtr<IEnumSTATPROPSTG> enumSTATPROPSTG;
  /*
  if (m_ArchiveHandler)
  {
    if (m_ArchiveHandler->EnumProperties(&enumSTATPROPSTG) != S_OK)
      throw 1;
  }
  else
  */
  {
    if (_folder.QueryInterface(&enumProperties) != S_OK)
      throw 1;
    if (enumProperties->EnumProperties(&enumSTATPROPSTG)!= S_OK)
      throw 1;
  }

  _needSaveInfo = true;
  
  STATPROPSTG srcProperty;
  while (enumSTATPROPSTG->Next(1, &srcProperty, NULL) == S_OK)
  {
    CItemProperty destProperty;
    destProperty.Type = srcProperty.vt;
    destProperty.ID = srcProperty.propid;
    if (srcProperty.propid == kpidIsFolder)
      continue;
    {
      if (srcProperty.lpwstrName != NULL)
        destProperty.Name = srcProperty.lpwstrName;
      else
        destProperty.Name = L"Error";
    }
    if (srcProperty.lpwstrName != NULL)
      CoTaskMemFree(srcProperty.lpwstrName);
    CSysString propName = GetNameOfProperty(srcProperty.propid);
    if (!propName.IsEmpty())
      destProperty.Name = GetUnicodeString(propName);
    destProperty.Order = -1;
    destProperty.IsVisible = true;
    destProperty.Width = 100;
    _properties.Add(destProperty);
  }
  // InitColumns2(sortID);

  while(true)
    if (!_listView.DeleteColumn(0))
      break;

  int order = 0;
  for(int  i = 0; i < _listViewInfo.Columns.Size(); i++)
  {
    const CColumnInfo &columnInfo = _listViewInfo.Columns[i];
    int index = _properties.FindItemWithID(columnInfo.PropID);
    if (index >= 0)
    {
      CItemProperty &item = _properties[index];
      item.IsVisible = columnInfo.IsVisible;
      item.Width = columnInfo.Width;
      if (columnInfo.IsVisible)
        item.Order = order++;
      continue;
    }
  }
  for(i = 0; i < _properties.Size(); i++)
  {
    CItemProperty &item = _properties[i];
    if (item.Order < 0)
      item.Order = order++;
  }

  _visibleProperties.Clear();
  for (i = 0; i < _properties.Size(); i++)
  {
    const CItemProperty &property = _properties[i];
    if (property.IsVisible)
      _visibleProperties.Add(property);
  }

  // _sortIndex = 0;
  _sortID = kpidName;
  /*
  if (_listViewInfo.SortIndex >= 0)
  {
    int sortIndex = _properties.FindItemWithID(sortID);
    if (sortIndex >= 0)
      _sortIndex = sortIndex;
  }
  */
  _sortID = _listViewInfo.SortID;

  for (i = 0; i < _visibleProperties.Size(); i++)
  {
    InsertColumn(i);
  }
}

void CPanel::InsertColumn(int index)
{
  const CItemProperty &property = _visibleProperties[index];
  LV_COLUMN column;
  column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
  TCHAR string[1024];
  column.pszText = string;
  column.cx = property.Width;
  column.fmt = GetColumnAlign(property.ID, property.Type);
  column.iOrder = property.Order;
  column.iSubItem = index;
  CSysString propertyName = GetNameOfProperty(property.ID);
  if (propertyName.IsEmpty())
    propertyName = GetSystemString(property.Name);
  lstrcpy(string, propertyName);
  _listView.InsertColumn(index, &column);
}


void CPanel::RefreshListCtrl()
{
  RefreshListCtrl(UString(), 0, UStringVector());
}

int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData);


void CPanel::GetSelectedNames(UStringVector &selectedNames)
{
  selectedNames.Clear();
  /*
  CRecordVector<UINT32> indices;
  GetSelectedItemsIndexes(indices);
  selectedNames.Reserve(indices.Size());
  for (int  i = 0; i < indices.Size(); i++)
    selectedNames.Add(GetItemName(indices[i]));
  */
  for (int i = 0; i < _listView.GetItemCount(); i++)
  {
    const int kSize = 1024;
    TCHAR name[kSize + 1];
    LVITEM item;
    item.iItem = i;
    item.pszText = name;
    item.cchTextMax  = kSize;
    item.iSubItem = 0;
    item.mask = LVIF_TEXT | LVIF_PARAM;
    if (!_listView.GetItem(&item))
      continue;
    int realIndex = GetRealIndex(item);
    if (realIndex == -1)
      continue;
    if (_selectedStatusVector[realIndex])
      selectedNames.Add(GetUnicodeString(item.pszText));
  }
  selectedNames.Sort();
}

void CPanel::RefreshListCtrlSaveFocused()
{
  int focusedItem = _listView.GetFocusedItem();
  UString focusedName;
  if (focusedItem >= 0)
  {
    /*
    LPARAM param;
    if (_listView.GetItemParam(focusedItem, param))
      // focusedName = m_Files[param].Name;
      focusedName = GetItemName(param);
    */
    const int kSize = 1024;
    TCHAR name[kSize + 1];
    LVITEM item;
    item.iItem = focusedItem;
    item.pszText = name;
    item.cchTextMax  = kSize;
    item.iSubItem = 0;
    item.mask = LVIF_TEXT;
    if (_listView.GetItem(&item))
      focusedName = GetUnicodeString(item.pszText);
  }
  UStringVector selectedNames;
  GetSelectedNames(selectedNames);
  RefreshListCtrl(focusedName, focusedItem, selectedNames);
}

void CPanel::RefreshListCtrl(const UString &focusedName, int focusedPos,
    const UStringVector &selectedNames)
{
  LoadFullPathAndShow();
  // OutputDebugStringA("=======\n");
  // OutputDebugStringA("s1 \n");
  CDisableTimerProcessing timerProcessing(*this);

  if (focusedPos < 0)
    focusedPos = 0;

  _listView.SetRedraw(false);
  // m_RedrawEnabled = false;

  LVITEM item;
  ZeroMemory(&item, sizeof(item));
  
  _listView.DeleteAllItems();
  _selectedStatusVector.Clear();
  _realIndices.Clear();
  _startGroupSelect = 0;

  _selectionIsDefined = false;
  
  // m_Files.Clear();
  // _folder.Release();

  if (!_folder)
  {
    // throw 1;
    SetToRootFolder();
  }
  
  bool isRoot = IsRootFolder();
  _headerToolBar.EnableButton(kParentFolderID, !IsRootFolder());

  if (_folder->LoadItems() != S_OK)
    return;

  InitColumns();


  // OutputDebugString(TEXT("Start Dir\n"));
  UINT32 numItems;
  _folder->GetNumberOfItems(&numItems);

  bool showDots = _showDots && !IsRootFolder();

  _listView.SetItemCount(numItems + (showDots ? 1 : 0));

  _selectedStatusVector.Reserve(numItems);
  int cursorIndex = -1;  

  CComPtr<IFolderGetSystemIconIndex> folderGetSystemIconIndex;
  if (!IsFSFolder() || _showRealFileIcons)
    _folder.QueryInterface(&folderGetSystemIconIndex);

  if (showDots)
  {
    UString itemName = L"..";
    item.iItem = _listView.GetItemCount();
    if (itemName.CompareNoCase(focusedName) == 0)
      cursorIndex = item.iItem;
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    int subItem = 0;
    item.iSubItem = subItem++;
    item.lParam = -1;
    const int kMaxNameSize = MAX_PATH * 2;
    TCHAR string[kMaxNameSize];
    lstrcpyn(string, GetSystemString(itemName), kMaxNameSize);
    item.pszText = string;
    UINT32 attributes = FILE_ATTRIBUTE_DIRECTORY;
    item.iImage = _extToIconMap.GetIconIndex(attributes, 
        GetSystemString(itemName));
    if (item.iImage < 0)
      item.iImage = 0;
    if(_listView.InsertItem(&item) == -1)
      return;
  }
  
  // OutputDebugStringA("S1\n");

  for(int i = 0; i < numItems; i++)
  {
    UString itemName = GetItemName(i);
    if (itemName.CompareNoCase(focusedName) == 0)
      cursorIndex = _listView.GetItemCount();
    bool selected = false;
    if (selectedNames.FindInSorted(itemName) >= 0)
      selected = true;
    _selectedStatusVector.Add(selected);
    /*
    if (_virtualMode)
    {
      _realIndices.Add(i);
    }
    else
    */
    {

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    // item.mask = LVIF_TEXT | LVIF_PARAM;

  
    int subItem = 0;
    item.iItem = _listView.GetItemCount();
    
    item.iSubItem = subItem++;
    item.lParam = i;
    
    const int kMaxNameSize = MAX_PATH * 2;
    TCHAR string[kMaxNameSize];
    lstrcpyn(string, GetSystemString(itemName), kMaxNameSize);
    item.pszText = string;

    NCOM::CPropVariant propVariant;
    _folder->GetProperty(i, kpidAttributes, &propVariant);
    UINT32 attributes = 0;
    if (propVariant.vt == VT_UI4)
      attributes = propVariant.ulVal;
    else
    {
      if (IsItemFolder(i))
        attributes |= FILE_ATTRIBUTE_DIRECTORY;
    }

    bool defined  = false;

    if (folderGetSystemIconIndex)
    {
      folderGetSystemIconIndex->GetSystemIconIndex(i, &item.iImage);
      defined = (item.iImage > 0);
    }
    if (!defined)
    {
      if (_currentFolderPrefix.IsEmpty())
      {
        item.iImage = GetRealIconIndex(
            attributes, (GetSystemString(itemName) + TEXT("\\")));
      }
      else
      {
        item.iImage = _extToIconMap.GetIconIndex(attributes, 
            GetSystemString(itemName));
      }
    }
    if (item.iImage < 0)
      item.iImage = 0;

    if(_listView.InsertItem(&item) == -1)
      return; // error
    }
  }
  // OutputDebugStringA("End2\n");

  if(_listView.GetItemCount() > 0 && cursorIndex >= 0)
  {
    UINT state = LVIS_FOCUSED | LVIS_SELECTED;
    _listView.SetItemState(cursorIndex, state, state);
  }
  _listView.SortItems(CompareItems, (LPARAM)this);
  if (cursorIndex < 0 && _listView.GetItemCount() > 0)
  {
    if (focusedPos >= _listView.GetItemCount())
      focusedPos = _listView.GetItemCount() - 1;
    UINT state = LVIS_FOCUSED | LVIS_SELECTED;
    _listView.SetItemState(focusedPos, state, state);
  }
  // m_RedrawEnabled = true;
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
  _listView.SetRedraw(true);
  _listView.InvalidateRect(NULL, true);
  // OutputDebugStringA("End1\n");
  /*
  _listView.UpdateWindow();
  */
}

void CPanel::GetSelectedItemsIndices(CRecordVector<UINT32> &indices) const
{
  indices.Clear();
  /*
  int itemIndex = -1;
  while ((itemIndex = _listView.GetNextItem(itemIndex, LVNI_SELECTED)) != -1)
  {
    LPARAM param;
    if (_listView.GetItemParam(itemIndex, param))
      indices.Add(param);
  }
  */
  for (int i = 0; i < _selectedStatusVector.Size(); i++)
    if (_selectedStatusVector[i])
      indices.Add(i);
  indices.Sort();
}

void CPanel::GetOperatedItemIndices(CRecordVector<UINT32> &indices) const
{
  GetSelectedItemsIndices(indices);
  if (!indices.IsEmpty())
    return;
  if (_listView.GetSelectedCount() == 0)
    return;
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem >= 0)
  {
    int realIndex = GetRealItemIndex(focusedItem);
    if (realIndex != -1)
      indices.Add(realIndex);
  }
}

void CPanel::EditItem()
{
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  if (realIndex == -1)
    return;
  if (!IsItemFolder(realIndex))
    EditItem(realIndex);
}

void CPanel::OpenFocusedItemAsInternal()
{
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  if (IsItemFolder(realIndex))
    OpenFolder(realIndex);
  else
    OpenItem(realIndex, true, false);
}

void CPanel::OpenSelectedItems(bool tryInternal)
{
  CRecordVector<UINT32> indices;
  GetOperatedItemIndices(indices);
  if (indices.Size() > 20)
  {
    MessageBox(L"Too much items");
    return;
  }
  
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem >= 0)
  {
    int realIndex = GetRealItemIndex(focusedItem);
    if (realIndex == -1 && (tryInternal || indices.Size() == 0))
      indices.Insert(0, realIndex);
  }

  bool dirIsStarted = false;
  for(int i = 0; i < indices.Size(); i++)
  {
    UINT32 index = indices[i];
    // CFileInfo &aFile = m_Files[index];
    if (IsItemFolder(index))
    {
      if (!dirIsStarted)
      {
        if (tryInternal)
        {
          OpenFolder(index);
          dirIsStarted = true;
          break;
        }
        else
          OpenFolderExternal(index);
      }
    }
    else
      OpenItem(index, (tryInternal && indices.Size() == 1), true);
  }
}

UString CPanel::GetItemName(int itemIndex) const
{
  if (itemIndex == -1)
    return L"..";
  NCOM::CPropVariant propVariant;
  if (_folder->GetProperty(itemIndex, kpidName, &propVariant) != S_OK)
    throw 2723400;
  if (propVariant.vt != VT_BSTR)
    throw 2723401;
  return (propVariant.bstrVal);
}


bool CPanel::IsItemFolder(int itemIndex) const
{
  if (itemIndex == -1)
    return true;
  NCOM::CPropVariant propVariant;
  if (_folder->GetProperty(itemIndex, kpidIsFolder, &propVariant) != S_OK)
    throw 2723400;
  if (propVariant.vt == VT_BOOL)
    return VARIANT_BOOLToBool(propVariant.boolVal);
  if (propVariant.vt == VT_EMPTY)
    return false;
  throw 21632;
}

UINT64 CPanel::GetItemSize(int itemIndex) const
{
  if (itemIndex == -1)
    return 0;
  NCOM::CPropVariant propVariant;
  if (_folder->GetProperty(itemIndex, kpidSize, &propVariant) != S_OK)
    throw 2723400;
  if (propVariant.vt == VT_EMPTY)
    return 0;
  return ConvertPropVariantToUINT64(propVariant);
}

void CPanel::ReadListViewInfo()
{
  CComPtr<IFolderGetTypeID> folderGetTypeID;
  if(_folder.QueryInterface(&folderGetTypeID) != S_OK)
    return;
  CComBSTR typeID;
  folderGetTypeID->GetTypeID(&typeID);
  _typeIDString = GetSystemString((const wchar_t *)typeID);
  ::ReadListViewInfo(_typeIDString, _listViewInfo);
}

void CPanel::SaveListViewInfo()
{
  for(int i = 0; i < _visibleProperties.Size(); i++)
  {
    CItemProperty &property = _visibleProperties[i];
    LVCOLUMN winColumnInfo;
    winColumnInfo.mask = LVCF_ORDER | LVCF_WIDTH;
    if (!_listView.GetColumn(i, &winColumnInfo))
      throw 1;
    property.Order = winColumnInfo.iOrder;
    property.Width = winColumnInfo.cx;
  }

  CListViewInfo viewInfo;
  
  // PROPID sortPropID = _properties[_sortIndex].ID;
  PROPID sortPropID = _sortID;
  
  _visibleProperties.Sort();
  for(i = 0; i < _visibleProperties.Size(); i++)
  {
    const CItemProperty &property = _visibleProperties[i];
    CColumnInfo columnInfo;
    columnInfo.IsVisible = property.IsVisible;
    columnInfo.PropID = property.ID;
    columnInfo.Width = property.Width;
    viewInfo.Columns.Add(columnInfo);
  }
  for(i = 0; i < _properties.Size(); i++)
  {
    const CItemProperty &property = _properties[i];
    if (!property.IsVisible)
    {
      CColumnInfo columnInfo;
      columnInfo.IsVisible = property.IsVisible;
      columnInfo.PropID = property.ID;
      columnInfo.Width = property.Width;
      viewInfo.Columns.Add(columnInfo);
    }
  }
  
  // viewInfo.SortIndex = viewInfo.FindColumnWithID(sortPropID);
  viewInfo.SortID = sortPropID;

  viewInfo.Ascending = _ascending;
  if (!_listViewInfo.IsEqual(viewInfo))
  {
    ::SaveListViewInfo(_typeIDString, viewInfo);
    _listViewInfo = viewInfo;
  }
}

bool CPanel::OnRightClick(LPNMITEMACTIVATE itemActiveate, LRESULT &result)
{
  if(itemActiveate->hdr.hwndFrom == HWND(_listView))
    return false;

  POINT point;
  ::GetCursorPos(&point);

  CMenu menu;
  CMenuDestroyer menuDestroyer(menu);

  menu.CreatePopup();

  const int kCommandStart = 100;
  for(int i = 0; i < _properties.Size(); i++)
  {
    const CItemProperty &property = _properties[i];
    UINT flags =  MF_STRING;
    if (property.IsVisible)
      flags |= MF_CHECKED;
    if (i == 0)
      flags |= MF_GRAYED;
    menu.AppendItem(flags, kCommandStart + i, GetSystemString(property.Name));
  }
  int menuResult = menu.Track(TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY, 
      point.x, point.y, _listView);
  if (menuResult >= kCommandStart && menuResult <= kCommandStart + _properties.Size())
  {
    int index = menuResult - kCommandStart;
    CItemProperty &property = _properties[index];
    property.IsVisible = !property.IsVisible;

    if (property.IsVisible)
    {
      int prevVisibleSize = _visibleProperties.Size();
      property.Order = prevVisibleSize;
      _visibleProperties.Add(property);
      InsertColumn(prevVisibleSize);
    }
    else
    {
      int visibleIndex = _visibleProperties.FindItemWithID(property.ID);
      _visibleProperties.Delete(visibleIndex);
      /*
      if (_sortIndex == index)
      {
        _sortIndex = 0;
        _ascending = true;
      }
      */
      if (_sortID == property.ID)
      {
        _sortID = kpidName;
        _ascending = true;
      }

      _listView.DeleteColumn(visibleIndex);
    }
  }
  result = TRUE;
  return true;
}

void CPanel::OnReload()
{
  RefreshListCtrlSaveFocused();
  OnRefreshStatusBar();
}

void CPanel::OnTimer()
{
  if (!_processTimer)
    return;
  CComPtr<IFolderWasChanged> folderWasChanged;
  if (_folder.QueryInterface(&folderWasChanged) != S_OK)
    return;
  INT32 wasChanged;
  if (folderWasChanged->WasChanged(&wasChanged) != S_OK)
    return;
  if (wasChanged == 0)
    return;
  OnReload();
}

