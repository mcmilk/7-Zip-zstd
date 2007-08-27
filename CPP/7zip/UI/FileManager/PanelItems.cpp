// PanelItems.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/Menu.h"

#include "../../PropID.h"

#include "Panel.h"
#include "resource.h"

#include "RootFolder.h"

#include "PropertyName.h"
#include "LangUtils.h"

extern "C"
{
  #include "../../../../C/Sort.h"
}

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

  _needSaveInfo = true;

  UInt32 numProperties;
  _folder->GetNumberOfProperties(&numProperties);  
  int i;
  for (i = 0; i < (int)numProperties; i++)
  {
    CMyComBSTR name;
    PROPID propID;
    VARTYPE varType;

    if (_folder->GetPropertyInfo(i, &name, &propID, &varType) != S_OK)
      throw 1;

    CItemProperty destProperty;
    destProperty.Type = varType;
    destProperty.ID = propID;
    if (propID == kpidIsFolder)
      continue;
    {
      if (name != NULL)
        destProperty.Name = name;
      else
        destProperty.Name = L"Error";
    }
    UString propName = GetNameOfProperty(propID);
    if (!propName.IsEmpty())
      destProperty.Name = propName;
    destProperty.Order = -1;
    destProperty.IsVisible = true;
    destProperty.Width = 100;
    _properties.Add(destProperty);
  }
  // InitColumns2(sortID);

  for (;;)
    if (!_listView.DeleteColumn(0))
      break;

  int order = 0;
  for(i = 0; i < _listViewInfo.Columns.Size(); i++)
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
  LV_COLUMNW column;
  column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;
  column.cx = property.Width;
  column.fmt = GetColumnAlign(property.ID, property.Type);
  column.iOrder = property.Order;
  column.iSubItem = index;
  UString propertyName = GetNameOfProperty(property.ID);
  if (propertyName.IsEmpty())
    propertyName = property.Name;
  column.pszText = (wchar_t *)(const wchar_t *)propertyName;
  _listView.InsertColumn(index, &column);
}

void CPanel::RefreshListCtrl()
{
  RefreshListCtrl(UString(), -1, true, UStringVector());
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

void CPanel::RefreshListCtrl(const CSelectedState &s)
{
  bool selectFocused = s.SelectFocused;
  if (_mySelectMode)
    selectFocused = true;
  RefreshListCtrl(s.FocusedName, s.FocusedItem, selectFocused, s.SelectedNames);
}

void CPanel::RefreshListCtrlSaveFocused()
{
  CSelectedState state;
  SaveSelectedState(state);
  RefreshListCtrl(state);
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

void CPanel::RefreshListCtrl(const UString &focusedName, int focusedPos, bool selectFocused,
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

  if (_folder->LoadItems() != S_OK)
    return;

  InitColumns();


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
    item.pszText = (wchar_t *)(const wchar_t *)itemName;
    UInt32 attributes = FILE_ATTRIBUTE_DIRECTORY;
    item.iImage = _extToIconMap.GetIconIndex(attributes, itemName);
    if (item.iImage < 0)
      item.iImage = 0;
    if(_listView.InsertItem(&item) == -1)
      return;
  }
  
  // OutputDebugStringA("S1\n");

  for(UInt32 i = 0; i < numItems; i++)
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
      item.pszText = (wchar_t *)(const wchar_t *)correctedName;
    }
    else
      item.pszText = (wchar_t *)(const wchar_t *)itemName;

    NCOM::CPropVariant propVariant;
    _folder->GetProperty(i, kpidAttributes, &propVariant);
    UInt32 attributes = 0;
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
        int iconIndexTemp;
        GetRealIconIndex(itemName + L"\\", attributes, iconIndexTemp);
        item.iImage = iconIndexTemp;
      }
      else
      {
        item.iImage = _extToIconMap.GetIconIndex(attributes, itemName);
      }
    }
    if (item.iImage < 0)
      item.iImage = 0;

    if(_listView.InsertItem(&item) == -1)
      return; // error
  }
  // OutputDebugStringA("End2\n");

  if(_listView.GetItemCount() > 0 && cursorIndex >= 0)
    SetFocusedSelectedItem(cursorIndex, selectFocused);
  _listView.SortItems(CompareItems, (LPARAM)this);
  if (cursorIndex < 0 && _listView.GetItemCount() > 0)
  {
    if (focusedPos >= _listView.GetItemCount())
      focusedPos = _listView.GetItemCount() - 1;
    SetFocusedSelectedItem(focusedPos, true);
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
    if(_listView.GetItemState(focusedItem, LVIS_SELECTED) == LVIS_SELECTED)
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
    MessageBox(LangString(IDS_TOO_MANY_ITEMS, 0x02000606));
    return;
  }
  
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem >= 0)
  {
    int realIndex = GetRealItemIndex(focusedItem);
    if (realIndex == kParentIndex && (tryInternal || indices.Size() == 0))
      indices.Insert(0, realIndex);
  }

  bool dirIsStarted = false;
  for(int i = 0; i < indices.Size(); i++)
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
  NCOM::CPropVariant propVariant;
  if (_folder->GetProperty(itemIndex, kpidName, &propVariant) != S_OK)
    throw 2723400;
  if (propVariant.vt != VT_BSTR)
    throw 2723401;
  return (propVariant.bstrVal);
}

UString CPanel::GetItemPrefix(int itemIndex) const
{
  if (itemIndex == kParentIndex)
    return UString();
  NCOM::CPropVariant propVariant;
  if (_folder->GetProperty(itemIndex, kpidPrefix, &propVariant) != S_OK)
    throw 2723400;
  UString prefix;
  if (propVariant.vt == VT_BSTR)
    prefix = propVariant.bstrVal;
  return prefix;
}

UString CPanel::GetItemRelPath(int itemIndex) const
{
  return GetItemPrefix(itemIndex) + GetItemName(itemIndex);
}


bool CPanel::IsItemFolder(int itemIndex) const
{
  if (itemIndex == kParentIndex)
    return true;
  NCOM::CPropVariant propVariant;
  if (_folder->GetProperty(itemIndex, kpidIsFolder, &propVariant) != S_OK)
    throw 2723400;
  if (propVariant.vt == VT_BOOL)
    return VARIANT_BOOLToBool(propVariant.boolVal);
  if (propVariant.vt == VT_EMPTY)
    return false;
  return false;
}

UINT64 CPanel::GetItemSize(int itemIndex) const
{
  if (itemIndex == kParentIndex)
    return 0;
  NCOM::CPropVariant propVariant;
  if (_folder->GetProperty(itemIndex, kpidSize, &propVariant) != S_OK)
    throw 2723400;
  if (propVariant.vt == VT_EMPTY)
    return 0;
  return ConvertPropVariantToUInt64(propVariant);
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
  for(i = 0; i < _visibleProperties.Size(); i++)
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
  CMyComPtr<IFolderWasChanged> folderWasChanged;
  if (_folder.QueryInterface(IID_IFolderWasChanged, &folderWasChanged) != S_OK)
    return;
  INT32 wasChanged;
  if (folderWasChanged->WasChanged(&wasChanged) != S_OK)
    return;
  if (wasChanged == 0)
    return;
  OnReload();
}

