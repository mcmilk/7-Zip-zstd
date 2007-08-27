// PanelSelect.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/StringConvert.h"
#include "Common/Wildcard.h"

#include "Panel.h"

#include "ComboDialog.h"

#include "LangUtils.h"

void CPanel::OnShiftSelectMessage()
{
  if (!_mySelectMode)
    return;
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  if (!_selectionIsDefined)
    return;
  int startItem = MyMin(focusedItem, _prevFocusedItem);
  int finishItem = MyMax(focusedItem, _prevFocusedItem);
  for (int i = 0; i < _listView.GetItemCount(); i++)
  {
    int realIndex = GetRealItemIndex(i);
    if (realIndex == kParentIndex)
      continue;
    if (i >= startItem && i <= finishItem)
      if (_selectedStatusVector[realIndex] != _selectMark)
      {
        _selectedStatusVector[realIndex] = _selectMark;
        _listView.RedrawItem(i);
      }
  }
  _prevFocusedItem = focusedItem;
}

void CPanel::OnArrowWithShift()
{
  if (!_mySelectMode)
    return;
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  if (_selectionIsDefined)
  {
    if (realIndex != kParentIndex)
      _selectedStatusVector[realIndex] = _selectMark;
  }
  else
  {
    if (realIndex == kParentIndex)
    {
      _selectionIsDefined = true;
      _selectMark = true;
    }
    else
    {
      _selectionIsDefined = true;
      _selectMark = !_selectedStatusVector[realIndex];
      _selectedStatusVector[realIndex] = _selectMark;
    }
  }
  _prevFocusedItem = focusedItem;
  PostMessage(kShiftSelectMessage);
  _listView.RedrawItem(focusedItem);
}

void CPanel::OnInsert()
{
  /*
  const int kState = CDIS_MARKED; // LVIS_DROPHILITED;
  UINT state = (_listView.GetItemState(focusedItem, LVIS_CUT) == 0) ?
      LVIS_CUT : 0;
  _listView.SetItemState(focusedItem, state, LVIS_CUT);
  // _listView.SetItemState(focusedItem, LVIS_SELECTED, LVIS_SELECTED);

  */
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  bool isSelected = !_selectedStatusVector[realIndex];
  if (realIndex != kParentIndex)
    _selectedStatusVector[realIndex] = isSelected;
  
  if (!_mySelectMode)
    _listView.SetItemState(focusedItem, isSelected ? LVIS_SELECTED: 0, LVIS_SELECTED);

  _listView.RedrawItem(focusedItem);

  int nextIndex = focusedItem + 1;
  if (nextIndex < _listView.GetItemCount())
  {
    _listView.SetItemState(nextIndex, LVIS_FOCUSED | LVIS_SELECTED, 
        LVIS_FOCUSED | LVIS_SELECTED);
    _listView.EnsureVisible(nextIndex, false);
  }
}

/*
void CPanel::OnUpWithShift()
{
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int index = GetRealItemIndex(focusedItem);
  _selectedStatusVector[index] = !_selectedStatusVector[index];
  _listView.RedrawItem(index);
}

void CPanel::OnDownWithShift()
{
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int index = GetRealItemIndex(focusedItem);
  _selectedStatusVector[index] = !_selectedStatusVector[index];
  _listView.RedrawItem(index);
}
*/

void CPanel::UpdateSelection()
{
  if (!_mySelectMode)
  {
    bool enableTemp = _enableItemChangeNotify;
    _enableItemChangeNotify = false;
    int numItems = _listView.GetItemCount();
    for (int i = 0; i < numItems; i++)
    {
      int realIndex = GetRealItemIndex(i);
      if (realIndex != kParentIndex)
      {
        UINT value = 0;
        value = _selectedStatusVector[realIndex] ? LVIS_SELECTED: 0;
        _listView.SetItemState(i, value, LVIS_SELECTED);
      }
    }
    _enableItemChangeNotify = enableTemp;
  }
  _listView.RedrawAllItems();
}


void CPanel::SelectSpec(bool selectMode)
{
  CComboDialog comboDialog;
  comboDialog.Title = selectMode ? 
      LangString(IDS_SELECT, 0x03020250):
      LangString(IDS_DESELECT, 0x03020251);
  comboDialog.Static = LangString(IDS_SELECT_MASK, 0x03020252);
  comboDialog.Value = L"*";
  if (comboDialog.Create(GetParent()) == IDCANCEL)
    return;
  const UString &mask = comboDialog.Value;
  for (int i = 0; i < _selectedStatusVector.Size(); i++)
    if (CompareWildCardWithName(mask, GetItemName(i)))
       _selectedStatusVector[i] = selectMode;
  UpdateSelection();
}

void CPanel::SelectByType(bool selectMode)
{
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  UString name = GetItemName(realIndex);
  bool isItemFolder = IsItemFolder(realIndex);

  /*
  UINT32 numItems;
  _folder->GetNumberOfItems(&numItems);
  if ((UInt32)_selectedStatusVector.Size() != numItems)
    throw 11111;
  */

  if (isItemFolder)
  {
    for (int i = 0; i < _selectedStatusVector.Size(); i++)
      if (IsItemFolder(i) == isItemFolder)
        _selectedStatusVector[i] = selectMode;
  }
  else
  {
    int pos = name.ReverseFind(L'.');
    if (pos < 0)
    {
      for (int i = 0; i < _selectedStatusVector.Size(); i++)
        if (IsItemFolder(i) == isItemFolder && GetItemName(i).ReverseFind(L'.') < 0)
          _selectedStatusVector[i] = selectMode;
    }
    else
    {
      UString mask = UString(L'*') + name.Mid(pos);
      for (int i = 0; i < _selectedStatusVector.Size(); i++)
        if (IsItemFolder(i) == isItemFolder && CompareWildCardWithName(mask, GetItemName(i)))
          _selectedStatusVector[i] = selectMode;
    }
  }
  UpdateSelection();
}

void CPanel::SelectAll(bool selectMode)
{
  for (int i = 0; i < _selectedStatusVector.Size(); i++)
    _selectedStatusVector[i] = selectMode;
  UpdateSelection();
}

void CPanel::InvertSelection()
{
  if (!_mySelectMode)
  {
    int numSelected = 0;
    for (int i = 0; i < _selectedStatusVector.Size(); i++)
      if (_selectedStatusVector[i])
        numSelected++;
    if (numSelected == 1)
    {
      int focused = _listView.GetFocusedItem();
      if (focused >= 0)
      {
        int realIndex = GetRealItemIndex(focused);
        if (realIndex >= 0)
          if (_selectedStatusVector[realIndex])
            _selectedStatusVector[realIndex] = false;
      }
    }
  }
  for (int i = 0; i < _selectedStatusVector.Size(); i++)
    _selectedStatusVector[i] = !_selectedStatusVector[i];
  UpdateSelection();
}

void CPanel::KillSelection()
{
  SelectAll(false);
  if (!_mySelectMode)
  {
    int focused = _listView.GetFocusedItem();
    if (focused >= 0)
      _listView.SetItemState(focused, LVIS_SELECTED, LVIS_SELECTED);
  }
}

void CPanel::OnLeftClick(LPNMITEMACTIVATE itemActivate)
{
  if(itemActivate->hdr.hwndFrom != HWND(_listView))
    return;
  // It will be work only for Version 4.71 (IE 4);
  int indexInList = itemActivate->iItem;
  if (indexInList < 0)
    return;
  if ((itemActivate->uKeyFlags & LVKF_SHIFT) != 0)
  {
    // int focusedIndex = _listView.GetFocusedItem();
    int focusedIndex = _startGroupSelect;
    if (focusedIndex < 0)
      return;
    int startItem = MyMin(focusedIndex, indexInList);
    int finishItem = MyMax(focusedIndex, indexInList);
    for (int i = 0; i < _selectedStatusVector.Size(); i++)
    {
      int realIndex = GetRealItemIndex(i);
      if (realIndex == kParentIndex)
        continue;
      bool selected = (i >= startItem && i <= finishItem);
      if (_selectedStatusVector[realIndex] != selected)
      {
        _selectedStatusVector[realIndex] = selected;
        _listView.RedrawItem(i);
      }
    }
  }
  else 
  {
    _startGroupSelect = indexInList;
    if ((itemActivate->uKeyFlags & LVKF_CONTROL) != 0)
    {
      int realIndex = GetRealItemIndex(indexInList);
      if (realIndex != kParentIndex)
      {
        _selectedStatusVector[realIndex] = !_selectedStatusVector[realIndex];
        _listView.RedrawItem(indexInList);
      }
    }
  }
  return;
}

