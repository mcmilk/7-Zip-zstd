// PanelSelect.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/StringConvert.h"
#include "Common/Wildcard.h"

#include "Panel.h"

#include "Resource/ComboDialog/ComboDialog.h"

#include "LangUtils.h"

void CPanel::OnShiftSelectMessage()
{
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
    if (realIndex == -1)
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
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  if (_selectionIsDefined)
  {
    if (realIndex != -1)
      _selectedStatusVector[realIndex] = _selectMark;
  }
  else
  {
    if (realIndex == -1)
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
  if (realIndex != -1)
    _selectedStatusVector[realIndex] = !_selectedStatusVector[realIndex];
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

void CPanel::SelectSpec(bool selectMode)
{
  CComboDialog comboDialog;
  comboDialog.Title = selectMode ? 
      LangLoadStringW(IDS_SELECT, 0x03020250):
      LangLoadStringW(IDS_DESELECT, 0x03020251);
  comboDialog.Static = LangLoadStringW(IDS_SELECT_MASK, 0x03020252);
  comboDialog.Value = L"*";
  if (comboDialog.Create(GetParent()) == IDCANCEL)
    return;
  UString mask = GetUnicodeString(comboDialog.Value);
  for (int i = 0; i < _selectedStatusVector.Size(); i++)
    if (CompareWildCardWithName(mask, GetItemName(i)))
       _selectedStatusVector[i] = selectMode;
  _listView.RedrawAllItems();
}

void CPanel::SelectByType(bool selectMode)
{
  int focusedItem = _listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = GetRealItemIndex(focusedItem);
  UString name = GetItemName(realIndex);
  bool isItemFolder = IsItemFolder(realIndex);

  UINT32 numItems;
  _folder->GetNumberOfItems(&numItems);
  if ((UInt32)_selectedStatusVector.Size() != numItems)
    throw 11111;

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
  _listView.RedrawAllItems();
}

void CPanel::SelectAll(bool selectMode)
{
  for (int i = 0; i < _selectedStatusVector.Size(); i++)
    _selectedStatusVector[i] = selectMode;
  _listView.RedrawAllItems();
}

void CPanel::InvertSelection()
{
  for (int i = 0; i < _selectedStatusVector.Size(); i++)
    _selectedStatusVector[i] = !_selectedStatusVector[i];
  _listView.RedrawAllItems();
}

void CPanel::KillSelection()
{
  SelectAll(false);
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
      if (realIndex == -1)
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
      if (realIndex != -1)
      {
        _selectedStatusVector[realIndex] = !_selectedStatusVector[realIndex];
        _listView.RedrawItem(indexInList);
      }
    }
  }
  return;
}

