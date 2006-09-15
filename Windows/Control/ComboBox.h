// Windows/Control/ComboBox.h

#ifndef __WINDOWS_CONTROL_COMBOBOX_H
#define __WINDOWS_CONTROL_COMBOBOX_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

#include <commctrl.h>

namespace NWindows {
namespace NControl {

class CComboBox: public CWindow
{
public:
  void ResetContent() { SendMessage(CB_RESETCONTENT, 0, 0); }
  LRESULT AddString(LPCTSTR string) { return SendMessage(CB_ADDSTRING, 0, (LPARAM)string); }
  #ifndef _UNICODE
  LRESULT AddString(LPCWSTR string);
  #endif
  LRESULT SetCurSel(int index) { return SendMessage(CB_SETCURSEL, index, 0); }
  int GetCurSel() { return (int)SendMessage(CB_GETCURSEL, 0, 0); }
  int GetCount() { return (int)SendMessage(CB_GETCOUNT, 0, 0); }
  
  LRESULT GetLBTextLen(int index) { return SendMessage(CB_GETLBTEXTLEN, index, 0); }
  LRESULT GetLBText(int index, LPTSTR string) { return SendMessage(CB_GETLBTEXT, index, (LPARAM)string); }
  LRESULT GetLBText(int index, CSysString &s);
  #ifndef _UNICODE
  LRESULT GetLBText(int index, UString &s);
  #endif

  LRESULT SetItemData(int index, LPARAM lParam)
    { return SendMessage(CB_SETITEMDATA, index, lParam); }
  LRESULT GetItemData(int index)
    { return SendMessage(CB_GETITEMDATA, index, 0); }
};

class CComboBoxEx: public CWindow
{
public:
  LRESULT DeleteItem(int index)
    { return SendMessage(CBEM_DELETEITEM, index, 0); }
  LRESULT InsertItem(COMBOBOXEXITEM *item)
    { return SendMessage(CBEM_INSERTITEM, 0, (LPARAM)item); }
  DWORD SetExtendedStyle(DWORD exMask, DWORD exStyle)
    { return (DWORD)SendMessage(CBEM_SETEXTENDEDSTYLE, exMask, exStyle); }
  HWND GetEditControl()
    { return (HWND)SendMessage(CBEM_GETEDITCONTROL, 0, 0); }
};

}}

#endif