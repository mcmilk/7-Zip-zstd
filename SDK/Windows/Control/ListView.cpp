// Windows/Control/ListView.cpp

#include "StdAfx.h"

#include "Windows/Control/ListView.h"

namespace NWindows {
namespace NControl {

bool CListView::CreateEx(DWORD anExStyle, DWORD aStyle,
      int x, int y, int aWidth, int aHeight,
      HWND aParentWindow, HMENU anIDorHMenu, 
      HINSTANCE anInstance, LPVOID aCreateParam)
{
  return CWindow::CreateEx(anExStyle, WC_LISTVIEW, _T(""), aStyle, x, y, aWidth,
      aHeight, aParentWindow, anIDorHMenu, anInstance, aCreateParam);
}

bool CListView::GetItemParam(int anItemIndex, LPARAM &aParam) const 
{ 
  LVITEM anItem;
  anItem.iItem = anItemIndex;
  anItem.iSubItem = 0;
  anItem.mask = LVIF_PARAM;
  bool aResult = GetItem(&anItem);
  aParam = anItem.lParam;
  return aResult;
}

/*
int CListView::InsertItem(UINT aMask, int anItem, LPCTSTR lpszItem, 
    UINT nState, UINT nStateMask, int nImage, LPARAM lParam)
{
  LVITEM item;
  item.mask = nMask;
  item.iItem = nItem;
  item.iSubItem = 0;
  item.pszText = (LPTSTR)lpszItem;
  item.state = nState;
  item.stateMask = nStateMask;
  item.iImage = nImage;
  item.lParam = lParam;
  return InsertItem(&item);
}

int CListView::InsertItem(int nItem, LPCTSTR lpszItem)
{ 
  return InsertItem(LVIF_TEXT, nItem, lpszItem, 0, 0, 0, 0); 
}

int CListView::InsertItem(int nItem, LPCTSTR lpszItem, int nImage)
{ 
  return InsertItem(LVIF_TEXT | LVIF_IMAGE, nItem, lpszItem, 0, 0, nImage, 0); 
}
*/

}}

