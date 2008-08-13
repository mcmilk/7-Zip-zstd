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

bool CListView::GetItemParam(int index, LPARAM &param) const
{
  LVITEM item;
  item.iItem = index;
  item.iSubItem = 0;
  item.mask = LVIF_PARAM;
  bool aResult = GetItem(&item);
  param = item.lParam;
  return aResult;
}

int CListView::InsertColumn(int columnIndex, LPCTSTR text, int width)
{
  LVCOLUMN ci;
  ci.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  ci.pszText = (LPTSTR)text;
  ci.iSubItem = columnIndex;
  ci.cx = width;
  return InsertColumn(columnIndex, &ci);
}

int CListView::InsertItem(int index, LPCTSTR text)
{
  LVITEM item;
  item.mask = LVIF_TEXT | LVIF_PARAM;
  item.iItem = index;
  item.lParam = index;
  item.pszText = (LPTSTR)text;
  item.iSubItem = 0;
  return InsertItem(&item);
}

int CListView::SetSubItem(int index, int subIndex, LPCTSTR text)
{
  LVITEM item;
  item.mask = LVIF_TEXT;
  item.iItem = index;
  item.pszText = (LPTSTR)text;
  item.iSubItem = subIndex;
  return SetItem(&item);
}

#ifndef _UNICODE

int CListView::InsertColumn(int columnIndex, LPCWSTR text, int width)
{
  LVCOLUMNW ci;
  ci.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  ci.pszText = (LPWSTR)text;
  ci.iSubItem = columnIndex;
  ci.cx = width;
  return InsertColumn(columnIndex, &ci);
}

int CListView::InsertItem(int index, LPCWSTR text)
{
  LVITEMW item;
  item.mask = LVIF_TEXT | LVIF_PARAM;
  item.iItem = index;
  item.lParam = index;
  item.pszText = (LPWSTR)text;
  item.iSubItem = 0;
  return InsertItem(&item);
}

int CListView::SetSubItem(int index, int subIndex, LPCWSTR text)
{
  LVITEMW item;
  item.mask = LVIF_TEXT;
  item.iItem = index;
  item.pszText = (LPWSTR)text;
  item.iSubItem = subIndex;
  return SetItem(&item);
}

#endif

}}
