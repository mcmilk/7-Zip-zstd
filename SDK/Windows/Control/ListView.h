// Windows/Control/ListView.h

#pragma once

#ifndef __WINDOWS_CONTROL_LISTVIEW_H
#define __WINDOWS_CONTROL_LISTVIEW_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CListView: public NWindows::CWindow
{
public:
  bool CreateEx(DWORD anExStyle, DWORD aStyle,
      int x, int y, int aWidth, int aHeight,
      HWND aParentWindow, HMENU anIDorHMenu, 
      HINSTANCE anInstance, LPVOID aCreateParam);
  
  bool DeleteAllItems()
    { return BOOLToBool(ListView_DeleteAllItems(m_Window)); }
  int InsertColumn(int aColumnIndex, const LVCOLUMN *aColumnInfo)
    { return ListView_InsertColumn(m_Window, aColumnIndex, aColumnInfo); }
  bool DeleteColumn(int aColumnIndex)
    { return BOOLToBool(ListView_DeleteColumn(m_Window, aColumnIndex)); }

  int InsertItem(const LVITEM* anItem)
    { return ListView_InsertItem(m_Window, anItem); }
  bool SetItem(const LVITEM* anItem)
    { return BOOLToBool(ListView_SetItem(m_Window, anItem)); }
  bool DeleteItem(int anItemIndex)
    { return BOOLToBool(ListView_DeleteItem(m_Window, anItemIndex)); }

  UINT GetSelectedCount() const
    { return ListView_GetSelectedCount(m_Window); }
  int GetItemCount() const
    { return ListView_GetItemCount(m_Window); }

  INT GetSelectionMark()
    { return ListView_GetSelectionMark(m_Window); }

  void SetItemCount(int aNumItems)
    {  ListView_SetItemCount(m_Window, aNumItems); }
  void SetItemCountEx(int aNumItems, DWORD aFlags)
    {  ListView_SetItemCountEx(m_Window, aNumItems, aFlags); }

  int GetNextItem(int aStartIndex, UINT aFlags) const
    { return ListView_GetNextItem(m_Window, aStartIndex, aFlags); }
  int GetNextSelectedItem(int aStartIndex) const
    { return GetNextItem(aStartIndex, LVNI_SELECTED); }
  int GetFocusedItem() const
    { return GetNextItem(-1, LVNI_FOCUSED); }
  
  bool GetItem(LVITEM* anItem) const 
    { return BOOLToBool(ListView_GetItem(m_Window, anItem)); }
  bool GetItemParam(int anItemIndex, LPARAM &aParam) const;
  void GetItemText(int anItemIndex, int aSubItemIndex, LPTSTR aText, int aTextSizeMax) const 
    { ListView_GetItemText(m_Window, anItemIndex, aSubItemIndex, aText, aTextSizeMax); }
  bool SortItems(PFNLVCOMPARE aCompareFunction, LPARAM aDataParam)
    { return BOOLToBool(ListView_SortItems(m_Window, aCompareFunction, aDataParam)); }

  void SetItemState(int anIndex, UINT aState, UINT aMask)
    { ListView_SetItemState(m_Window, anIndex, aState, aMask); }
  UINT GetItemState(int anIndex, UINT aMask)
    { return ListView_GetItemState(m_Window, anIndex, aMask); }

  bool GetColumn(int ColumnIndex, LVCOLUMN* aColumnInfo) const
    { return BOOLToBool(ListView_GetColumn(m_Window, ColumnIndex, aColumnInfo)); }

  HIMAGELIST SetImageList(HIMAGELIST anImageList, int anImageListType)
    { return ListView_SetImageList(m_Window, anImageList, anImageListType); }		

  // version 4.70: NT5 | (NT4 + ie3) | w98 | (w95 + ie3)
  DWORD GetExtendedListViewStyle()
    { return ListView_GetExtendedListViewStyle(m_Window); }
  void SetExtendedListViewStyle(DWORD anExStyle)
    { ListView_SetExtendedListViewStyle(m_Window, anExStyle); }
  void SetExtendedListViewStyle(DWORD anExMask, DWORD anExStyle)
    { ListView_SetExtendedListViewStyleEx(m_Window, anExMask, anExStyle); }

  #ifndef _WIN32_WCE
  void SetCheckState(UINT anIndex, bool aCheckState)
    { ListView_SetCheckState(m_Window, anIndex, BoolToBOOL(aCheckState)); }
  #endif
  bool GetCheckState(UINT anIndex)
    { return BOOLToBool(ListView_GetCheckState(m_Window, anIndex)); }


  bool EnsureVisible(int anIndex, bool aPartialOK)
    { return BOOLToBool(ListView_EnsureVisible(m_Window, anIndex, BoolToBOOL(aPartialOK))); }

  bool GetItemRect(int anIndex, RECT *aRect, int code)
    { return BOOLToBool(ListView_GetItemRect(m_Window, anIndex, aRect, code)); }

  HWND GetEditControl()
    { return ListView_GetEditControl(m_Window) ; }
  HWND EditLabel(int anItemIndex)
    { return ListView_EditLabel(m_Window, anItemIndex) ; }

  bool RedrawItems(int aFirstIndex, int aLastIndex)
    { return BOOLToBool(ListView_RedrawItems(m_Window, aFirstIndex, aLastIndex)); }
  bool RedrawItem(int anIndex)
    { return RedrawItems(anIndex, anIndex); }
 
  int HitTest(LPLVHITTESTINFO anInfo)
    { return ListView_HitTest(m_Window, anInfo); }
};

}}
#endif