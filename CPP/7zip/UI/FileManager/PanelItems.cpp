// PanelItems.cpp

#include "StdAfx.h"

#include "../../../../C/Sort.h"

#include "Common/StringConvert.h"

#include "Windows/Menu.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"

#include "../../PropID.h"

#include "resource.h"

#include "LangUtils.h"
#include "Panel.h"
#include "PropertyName.h"
#include "RootFolder.h"

using namespace NWindows;

static int GetColumnAlign(PROPID propID, VARTYPE varType)
{
  switch(propID)
  {
    case kpidCTime:
    case kpidATime:
    case kpidMTime:
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

HRESULT CPanel::InitColumns()
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

  _needSaveInfo = true;

  UInt32 numProperties;
  _folder->GetNumberOfProperties(&numProperties);
  int i;
  for (i = 0; i < (int)numProperties; i++)
  {
    CMyComBSTR name;
    PROPID propID;
    VARTYPE varType;

    RINOK(_folder->GetPropertyInfo(i, &name, &propID, &varType));

    if (propID == kpidIsDir)
      continue;

    CItemProperty prop;
    prop.Type = varType;
    prop.ID = propID;
    prop.Name = GetNameOfProperty(propID, name);
    prop.Order = -1;
    prop.IsVisible = true;
    prop.Width = 100;
    _properties.Add(prop);
  }
  // InitColumns2(sortID);

  for (;;)
    if (!_listView.DeleteColumn(0))
      break;

  int order = 0;
  for (i = 0; i < _listViewInfo.Columns.Size(); i++)
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
  for (i = 0; i < _properties.Size(); i++)
  {
    CItemProperty &item = _properties[i];
    if (item.Order < 0)
      item.Order = order++;
  }

  _visibleProperties.Clear();
  for (i = 0; i < _properties.Size(); i++)
  {
    const CItemProperty &prop = _properties[i];
    if (prop.IsVisible)
      _visibleProperties.Add(prop);
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
  return S_OK;
}

void CPanel::InsertColumn(int index)
{
  const CItemProperty &prop = _visibleProperties[index];
  LV_COLUMNW column;
  column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
  column.cx = prop.Width;
  column.fmt = GetColumnAlign(prop.ID, prop.Type);
  column.iOrder = prop.Order;
  column.iSubItem = index;
  column.pszText = const_cast<wchar_t *>((const wchar_t *)prop.Name);
  _listView.InsertColumn(index, &column);
}

HRESULT CPanel::RefreshListCtrl()
{
  return RefreshListCtrl(UString(), -1, true, UStringVector());
}

int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData);


void CPanel::GetSelectedNames(UStringVector &selectedNames)
{
  selectedNames.Clear();

  CRecordVector<UInt32> indices;
  GetSelectedItemsIndices(indices);
  selectedNames.Reserve(indices.Size());
  for (int  i = 0; i < indices.Size(); i++)
    selectedNames.Add(GetItemRelPath(indices[i]));

  /*
  for (int i = 0; i < _listView.GetItemCount(); i++)
  {
    const int kSize = 1024;
    WCHAR name[kSize + 1];
    LVITEMW item;
    item.iItem = i;
    item.pszText = name;
    item.cchTextMax  = kSize;
    item.iSubItem = 0;
    item.mask = LVIF_TEXT | LVIF_PARAM;
    if (!_listView.GetItem(&item))
      continue;
    int realIndex = GetRealIndex(item);
    if (realIndex == kParentIndex)
      continue;
    if (_selectedStatusVector[realIndex])
      selectedNames.Add(item.pszText);
  }
  */
  selectedNames.Sort();
}

void CPanel::SaveSelectedState(CSelectedState &s)
{
  s.FocusedName.Empty();
  s.SelectedNames.Clear();
  s.FocusedItem = _listView.GetFocusedItem();
  {
    if (s.FocusedItem >= 0)
    {
      int realIndex = GetRealItemIndex(s.FocusedItem);
      if (realIndex != kParentIndex)
        s.FocusedName = GetItemRelPath(realIndex);
        /*
        const int kSize = 1024;
        WCHAR name[kSize + 1];
        LVITEMW item;
        item.iItem = focusedItem;
        item.pszText = name;
        item.cchTextMax  = kSize;
        item.iSubItem = 0;
        item.mask = LVIF_TEXT;
        if (_listView.GetItem(&item))
        focusedName = item.pszText;
      */
    }
  }
  GetSelectedNames(s.SelectedNames);
}

HRESULT CPanel::RefreshListCtrl(const CSelectedState &s)
{
  bool selectFocused = s.SelectFocused;
  if (_mySelectMode)
    selectFocused = true;
  return RefreshListCtrl(s.FocusedName, s.FocusedItem, selectFocused, s.SelectedNames);
}

HRESULT CPanel::RefreshListCtrlSaveFocused()
{
  CSelectedState state;
  SaveSelectedState(state);
  return RefreshListCtrl(state);
}

void CPanel::SetFocusedSelectedItem(int index, bool select)
{
  UINT state = LVIS_FOCUSED;
  if (select)
    state |= LVIS_SELECTED;
  _listView.SetItemState(index, state, state);
  if (!_mySelectMode && select)
  {
    int realIndex = GetRealItemIndex(index);
    if (realIndex != kParentIndex)
      _selectedStatusVector[realIndex] = true;
  }
}

HRESULT CPanel::RefreshListCtrl(const UString &focusedName, int focusedPos, bool selectFocused,
    const UStringVector &selectedNames)
{
  _dontShowMode = false;
  LoadFullPathAndShow();
  // OutputDebugStringA("=======\n");
  // OutputDebugStringA("s1 \n");
  CDisableTimerProcessing timerProcessing(*this);

  if (focusedPos < 0)
    focusedPos = 0;

  _listView.SetRedraw(false);
  // m_RedrawEnabled = false;

  LVITEMW item;
  ZeroMemory(&item, sizeof(item));
  
  _listView.DeleteAllItems();
  _selectedStatusVector.Clear();
  // _realIndices.Clear();
  _startGroupSelect = 0;

  _selectionIsDefined = false;
  
  // m_Files.Clear();
  // _folder.Release();

  if (!_folder)
  {
    // throw 1;
    SetToRootFolder();
  }
  
  _headerToolBar.EnableButton(kParentFolderID, !IsRootFolder());

  CMyComPtr<IFolderSetFlatMode> folderSetFlatMode;
  _folder.QueryInterface(IID_IFolderSetFlatMode, &folderSetFlatMode);
  if (folderSetFlatMode)
    folderSetFlatMode->SetFlatMode(BoolToInt(_flatMode));

  RINOK(_folder->LoadItems());
  RINOK(InitColumns());

  // OutputDebugString(TEXT("Start Dir\n"));
  UInt32 numItems;
  _folder->GetNumberOfItems(&numItems);

  bool showDots = _showDots && !IsRootFolder();

  _listView.SetItemCount(numItems + (showDots ? 1 : 0));

  _selectedStatusVector.Reserve(numItems);
  int cursorIndex = -1;

  CMyComPtr<IFolderGetSystemIconIndex> folderGetSystemIconIndex;
  if (!IsFSFolder() || _showRealFileIcons)
    _folder.QueryInterface(IID_IFolderGetSystemIconIndex, &folderGetSystemIconIndex);

  if (showDots)
  {
    UString itemName = L"..";
    item.iItem = _listView.GetItemCount();
    if (itemName.CompareNoCase(focusedName) == 0)
      cursorIndex = item.iItem;
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    int subItem = 0;
    item.iSubItem = subItem++;
    item.lParam = kParentIndex;
    item.pszText = const_cast<wchar_t *>((const wchar_t *)itemName);
    UInt32 attrib = FILE_ATTRIBUTE_DIRECTORY;
    item.iImage = _extToIconMap.GetIconIndex(attrib, itemName);
    if (item.iImage < 0)
      item.iImage = 0;
    if (_listView.InsertItem(&item) == -1)
      return E_FAIL;
  }
  
  // OutputDebugStringA("S1\n");

  for (UInt32 i = 0; i < numItems; i++)
  {
    UString itemName = GetItemName(i);
    const UString relPath = GetItemRelPath(i);
    if (relPath.CompareNoCase(focusedName) == 0)
      cursorIndex = _listView.GetItemCount();
    bool selected = false;
    if (selectedNames.FindInSorted(relPath) >= 0)
      selected = true;
    _selectedStatusVector.Add(selected);

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;

    if (!_mySelectMode)
      if (selected)
      {
        item.mask |= LVIF_STATE;
        item.state = LVIS_SELECTED;
      }
  
    int subItem = 0;
    item.iItem = _listView.GetItemCount();
    
    item.iSubItem = subItem++;
    item.lParam = i;
    
    UString correctedName;
    if (itemName.Find(L"     ") >= 0)
    {
      int pos = 0;
      for (;;)
      {
        int posNew = itemName.Find(L"     ", pos);
        if (posNew < 0)
        {
          correctedName += itemName.Mid(pos);
          break;
        }
        correctedName += itemName.Mid(pos, posNew - pos);
        correctedName += L" ... ";
        pos = posNew;
        while (itemName[++pos] == ' ');
      }
      item.pszText = const_cast<wchar_t *>((const wchar_t *)correctedName);
    }
    else
      item.pszText = const_cast<wchar_t *>((const wchar_t *)itemName);

    NCOM::CPropVariant prop;
    RINOK(_folder->GetProperty(i, kpidAttrib, &prop));
    UInt32 attrib = 0;
    if (prop.vt == VT_UI4)
      attrib = prop.ulVal;
    else if (IsItemFolder(i))
      attrib |= FILE_ATTRIBUTE_DIRECTORY;

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
        int iconIndexTemp;
        GetRealIconIndex(itemName + WCHAR_PATH_SEPARATOR, attrib, iconIndexTemp);
        item.iImage = iconIndexTemp;
      }
      else
      {
        item.iImage = _extToIconMap.GetIconIndex(attrib, itemName);
      }
    }
    if (item.iImage < 0)
      item.iImage = 0;

    if (_listView.InsertItem(&item) == -1)
      return E_FAIL; // error
  }
  // OutputDebugStringA("End2\n");

  if (_listView.GetItemCount() > 0 && cursorIndex >= 0)
    SetFocusedSelectedItem(cursorIndex, selectFocused);
  _listView.SortItems(CompareItems, (LPARAM)this);
  if (cursorIndex < 0 && _listView.GetItemCount() > 0)
  {
    if (focusedPos >= _listView.GetItemCount())
      focusedPos = _listView.GetItemCount() - 1;
    // we select item only in showDots mode.
    SetFocusedSelectedItem(focusedPos, showDots);
  }
  // m_RedrawEnabled = true;
  _listView.EnsureVisible(_listView.GetFocusedItem(), false);
  _listView.SetRedraw(true);
  _listView.InvalidateRect(NULL, true);
  // OutputDebugStringA("End1\n");
  /*
  _listView.UpdateWindow();
  */
  return S_OK;
}

void CPanel::GetSelectedItemsIndices(CRecordVector<UInt32> &indices) const
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
  HeapSort(&indices.Front(), indices.Size());
}

void CPanel::GetOperatedItemIndices(CRecordVector<UInt32> &indices) const
{
  GetSelectedItemsIndices(indices);
  if (!indices.IsEmpty())
    return;
  if (_listView.GetSelectedCount() == 0)
    return;
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem >= 0)
  {
    if (_listView.GetItemState(focusedItem, LVIS_SELECTED) == LVIS_SELECTED)
    {
      int realIndex = GetRealItemIndex(focusedItem);
      if (realIndex != kParentIndex)
        indices.Add(realIndex);
    }
  }
}

void CPanel::GetAllItemIndices(CRecordVector<UInt32> &indices) const
{
  indices.Clear();
  UInt32 numItems;
  if (_folder->GetNumberOfItems(&numItems) == S_OK)
    for (UInt32 i = 0; i < numItems; i++)
      indices.Add(i);
}

void CPanel::GetOperatedIndicesSmart(CRecordVector<UInt32> &indices) const
{
  GetOperatedItemIndices(indices);
  if (indices.IsEmpty() || (indices.Size() == 1 && indices[0] == (UInt32)(Int32)-1))
    GetAllItemIndices(indices);
}

/*
void CPanel::GetOperatedListViewIndices(CRecordVector<UInt32> &indices) const
{
  indices.Clear();
  int numItems = _listView.GetItemCount();
  for (int i = 0; i < numItems; i++)
  {
    int realIndex = GetRealItemIndex(i);
    if (realIndex >= 0)
      if (_selectedStatusVector[realIndex])
        indices.Add(i);
  }
  if (indices.IsEmpty())
  {
    int focusedItem = _listView.GetFocusedItem();
      if (focusedItem >= 0)
        indices.Add(focusedItem);
  }
}
*/

void CPanel::EditItem()
{
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  if (realIndex == kParentIndex)
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
  CRecordVector<UInt32> indices;
  GetOperatedItemIndices(indices);
  if (indices.Size() > 20)
  {
    MessageBoxErrorLang(IDS_TOO_MANY_ITEMS, 0x02000606);
    return;
  }
  
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem >= 0)
  {
    int realIndex = GetRealItemIndex(focusedItem);
    if (realIndex == kParentIndex && (tryInternal || indices.Size() == 0) &&
        _listView.GetItemState(focusedItem, LVIS_SELECTED) == LVIS_SELECTED)
      indices.Insert(0, realIndex);
  }

  bool dirIsStarted = false;
  for (int i = 0; i < indices.Size(); i++)
  {
    UInt32 index = indices[i];
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
  if (itemIndex == kParentIndex)
    return L"..";
  NCOM::CPropVariant prop;
  if (_folder->GetProperty(itemIndex, kpidName, &prop) != S_OK)
    throw 2723400;
  if (prop.vt != VT_BSTR)
    throw 2723401;
  return prop.bstrVal;
}

UString CPanel::GetItemPrefix(int itemIndex) const
{
  if (itemIndex == kParentIndex)
    return UString();
  NCOM::CPropVariant prop;
  if (_folder->GetProperty(itemIndex, kpidPrefix, &prop) != S_OK)
    throw 2723400;
  UString prefix;
  if (prop.vt == VT_BSTR)
    prefix = prop.bstrVal;
  return prefix;
}

UString CPanel::GetItemRelPath(int itemIndex) const
{
  return GetItemPrefix(itemIndex) + GetItemName(itemIndex);
}

UString CPanel::GetItemFullPath(int itemIndex) const
{
  return _currentFolderPrefix + GetItemRelPath(itemIndex);
}

bool CPanel::IsItemFolder(int itemIndex) const
{
  if (itemIndex == kParentIndex)
    return true;
  NCOM::CPropVariant prop;
  if (_folder->GetProperty(itemIndex, kpidIsDir, &prop) != S_OK)
    throw 2723400;
  if (prop.vt == VT_BOOL)
    return VARIANT_BOOLToBool(prop.boolVal);
  if (prop.vt == VT_EMPTY)
    return false;
  return false;
}

UINT64 CPanel::GetItemSize(int itemIndex) const
{
  if (itemIndex == kParentIndex)
    return 0;
  NCOM::CPropVariant prop;
  if (_folder->GetProperty(itemIndex, kpidSize, &prop) != S_OK)
    throw 2723400;
  if (prop.vt == VT_EMPTY)
    return 0;
  return ConvertPropVariantToUInt64(prop);
}

void CPanel::ReadListViewInfo()
{
  _typeIDString = GetFolderTypeID();
  if (!_typeIDString.IsEmpty())
    ::ReadListViewInfo(_typeIDString, _listViewInfo);
}

void CPanel::SaveListViewInfo()
{
  int i;
  for (i = 0; i < _visibleProperties.Size(); i++)
  {
    CItemProperty &prop = _visibleProperties[i];
    LVCOLUMN winColumnInfo;
    winColumnInfo.mask = LVCF_ORDER | LVCF_WIDTH;
    if (!_listView.GetColumn(i, &winColumnInfo))
      throw 1;
    prop.Order = winColumnInfo.iOrder;
    prop.Width = winColumnInfo.cx;
  }

  CListViewInfo viewInfo;
  
  // PROPID sortPropID = _properties[_sortIndex].ID;
  PROPID sortPropID = _sortID;
  
  _visibleProperties.Sort();
  for (i = 0; i < _visibleProperties.Size(); i++)
  {
    const CItemProperty &prop = _visibleProperties[i];
    CColumnInfo columnInfo;
    columnInfo.IsVisible = prop.IsVisible;
    columnInfo.PropID = prop.ID;
    columnInfo.Width = prop.Width;
    viewInfo.Columns.Add(columnInfo);
  }
  for (i = 0; i < _properties.Size(); i++)
  {
    const CItemProperty &prop = _properties[i];
    if (!prop.IsVisible)
    {
      CColumnInfo columnInfo;
      columnInfo.IsVisible = prop.IsVisible;
      columnInfo.PropID = prop.ID;
      columnInfo.Width = prop.Width;
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


bool CPanel::OnRightClick(MY_NMLISTVIEW_NMITEMACTIVATE *itemActiveate, LRESULT &result)
{
  if (itemActiveate->hdr.hwndFrom == HWND(_listView))
    return false;
  POINT point;
  ::GetCursorPos(&point);
  ShowColumnsContextMenu(point.x, point.y);
  result = TRUE;
  return true;
}

void CPanel::ShowColumnsContextMenu(int x, int y)
{

  CMenu menu;
  CMenuDestroyer menuDestroyer(menu);

  menu.CreatePopup();

  const int kCommandStart = 100;
  for (int i = 0; i < _properties.Size(); i++)
  {
    const CItemProperty &prop = _properties[i];
    UINT flags =  MF_STRING;
    if (prop.IsVisible)
      flags |= MF_CHECKED;
    if (i == 0)
      flags |= MF_GRAYED;
    menu.AppendItem(flags, kCommandStart + i, prop.Name);
  }
  int menuResult = menu.Track(TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY, x, y, _listView);
  if (menuResult >= kCommandStart && menuResult <= kCommandStart + _properties.Size())
  {
    int index = menuResult - kCommandStart;
    CItemProperty &prop = _properties[index];
    prop.IsVisible = !prop.IsVisible;

    if (prop.IsVisible)
    {
      int prevVisibleSize = _visibleProperties.Size();
      prop.Order = prevVisibleSize;
      _visibleProperties.Add(prop);
      InsertColumn(prevVisibleSize);
    }
    else
    {
      int visibleIndex = _visibleProperties.FindItemWithID(prop.ID);
      _visibleProperties.Delete(visibleIndex);
      /*
      if (_sortIndex == index)
      {
        _sortIndex = 0;
        _ascending = true;
      }
      */
      if (_sortID == prop.ID)
      {
        _sortID = kpidName;
        _ascending = true;
      }

      _listView.DeleteColumn(visibleIndex);
    }
  }
}

void CPanel::OnReload()
{
  HRESULT res = RefreshListCtrlSaveFocused();
  if (res != S_OK)
    MessageBoxError(res);
  OnRefreshStatusBar();
}

void CPanel::OnTimer()
{
  if (!_processTimer)
    return;
  CMyComPtr<IFolderWasChanged> folderWasChanged;
  if (_folder.QueryInterface(IID_IFolderWasChanged, &folderWasChanged) != S_OK)
    return;
  Int32 wasChanged;
  if (folderWasChanged->WasChanged(&wasChanged) != S_OK)
    return;
  if (wasChanged == 0)
    return;
  OnReload();
}
