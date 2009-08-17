// Windows/Control/ComboBox.h

#ifndef __WINDOWS_CONTROL_COMBOBOX_H
#define __WINDOWS_CONTROL_COMBOBOX_H

#include <commctrl.h>

#include "../Window.h"

namespace NWindows {
namespace NControl {

class CComboBox: public CWindow
{
public:
  void ResetContent() { SendMessage(CB_RESETCONTENT, 0, 0); }
  LRESULT AddString(LPCTSTR s) { return SendMessage(CB_ADDSTRING, 0, (LPARAM)s); }
  #ifndef _UNICODE
  LRESULT AddString(LPCWSTR s);
  #endif
  LRESULT SetCurSel(int index) { return SendMessage(CB_SETCURSEL, index, 0); }
  int GetCurSel() { return (int)SendMessage(CB_GETCURSEL, 0, 0); }
  int GetCount() { return (int)SendMessage(CB_GETCOUNT, 0, 0); }
  
  LRESULT GetLBTextLen(int index) { return SendMessage(CB_GETLBTEXTLEN, index, 0); }
  LRESULT GetLBText(int index, LPTSTR s) { return SendMessage(CB_GETLBTEXT, index, (LPARAM)s); }
  LRESULT GetLBText(int index, CSysString &s);
  #ifndef _UNICODE
  LRESULT GetLBText(int index, UString &s);
  #endif

  LRESULT SetItemData(int index, LPARAM lParam) { return SendMessage(CB_SETITEMDATA, index, lParam); }
  LRESULT GetItemData(int index) { return SendMessage(CB_GETITEMDATA, index, 0); }

  void ShowDropDown(bool show = true) { SendMessage(CB_SHOWDROPDOWN, show ? TRUE : FALSE, 0);  }
};

#ifndef UNDER_CE

class CComboBoxEx: public CComboBox
{
public:
  bool SetUnicodeFormat(bool fUnicode) { return LRESULTToBool(SendMessage(CBEM_SETUNICODEFORMAT, BOOLToBool(fUnicode), 0)); }

  LRESULT DeleteItem(int index) { return SendMessage(CBEM_DELETEITEM, index, 0); }
  LRESULT InsertItem(COMBOBOXEXITEM *item) { return SendMessage(CBEM_INSERTITEM, 0, (LPARAM)item); }
  #ifndef _UNICODE
  LRESULT InsertItem(COMBOBOXEXITEMW *item) { return SendMessage(CBEM_INSERTITEMW, 0, (LPARAM)item); }
  #endif

  LRESULT SetItem(COMBOBOXEXITEM *item) { return SendMessage(CBEM_SETITEM, 0, (LPARAM)item); }
  DWORD SetExtendedStyle(DWORD exMask, DWORD exStyle) { return (DWORD)SendMessage(CBEM_SETEXTENDEDSTYLE, exMask, exStyle); }
  HWND GetEditControl() { return (HWND)SendMessage(CBEM_GETEDITCONTROL, 0, 0); }
  HIMAGELIST SetImageList(HIMAGELIST imageList) { return (HIMAGELIST)SendMessage(CBEM_SETIMAGELIST, 0, (LPARAM)imageList); }
};

#endif

}}

#endif
