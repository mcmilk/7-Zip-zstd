// Windows/Control/ListView.cpp

#include "StdAfx.h"

#include "Windows/Control/ListView.h"

namespace NWindows {
namespace NControl {

bool CListView::CreateEx(DWORD exStyle, DWORD style,
      int x, int y, int width, int height,
      HWND parentWindow, HMENU idOrHMenu, 
      HINSTANCE instance, LPVOID createParam)
{
  return CWindow::CreateEx(exStyle, WC_LISTVIEW, TEXT(""), style, x, y, width,
      height, parentWindow, idOrHMenu, instance, createParam);
}

bool CListView::GetItemParam(int itemIndex, LPARAM &param) const 
{ 
  LVITEM item;
  item.iItem = itemIndex;
  item.iSubItem = 0;
  item.mask = LVIF_PARAM;
  bool aResult = GetItem(&item);
  param = item.lParam;
  return aResult;
}

/*
int CListView::InsertItem(UINT mask, int item, LPCTSTR itemText, 
    UINT nState, UINT nStateMask, int nImage, LPARAM lParam)
{
  LVITEM item;
  item.mask = nMask;
  item.iItem = nItem;
  item.iSubItem = 0;
  item.pszText = (LPTSTR)itemText;
  item.state = nState;
  item.stateMask = nStateMask;
  item.iImage = nImage;
  item.lParam = lParam;
  return InsertItem(&item);
}

int CListView::InsertItem(int nItem, LPCTSTR itemText)
{ 
  return InsertItem(LVIF_TEXT, nItem, itemText, 0, 0, 0, 0); 
}

int CListView::InsertItem(int nItem, LPCTSTR itemText, int nImage)
{ 
  return InsertItem(LVIF_TEXT | LVIF_IMAGE, nItem, itemText, 0, 0, nImage, 0); 
}
*/

}}

