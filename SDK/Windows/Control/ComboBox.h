// Windows/Control/ComboBox.h

#pragma once

#ifndef __WINDOWS_CONTROL_COMBOBOX_H
#define __WINDOWS_CONTROL_COMBOBOX_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CComboBox: public CWindow
{
public:
  void ResetContent()
    { SendMessage(CB_RESETCONTENT, 0, 0); }
  int AddString(LPCTSTR string)
    { return SendMessage(CB_ADDSTRING, 0, (LPARAM)string); }
  int SetCurSel(int index)
    { return SendMessage(CB_SETCURSEL, index, 0); }
  int GetCurSel()
    { return SendMessage(CB_GETCURSEL, 0, 0); }
  int GetCount()
    { return SendMessage(CB_GETCOUNT, 0, 0); }
  
  int GetLBTextLen(int index)
    { return SendMessage(CB_GETLBTEXTLEN, index, 0); }
  int GetLBText(int index, LPTSTR string)
    { return SendMessage(CB_GETLBTEXT, index, (LPARAM)string); }
  int GetLBText(int index, CSysString &string);

  int SetItemData(int index, LPARAM lParam)
    { return SendMessage(CB_SETITEMDATA, index, lParam); }
  int GetItemData(int index)
    { return SendMessage(CB_GETITEMDATA, index, 0); }
};

class CComboBoxEx: public CWindow
{
public:
  int DeleteItem(int index)
    { return SendMessage(CBEM_DELETEITEM, index, 0); }
  int InsertItem(COMBOBOXEXITEM *item)
    { return SendMessage(CBEM_INSERTITEM, 0, (LPARAM)item); }
  DWORD SetExtendedStyle(DWORD exMask, DWORD exStyle)
    { return SendMessage(CBEM_SETEXTENDEDSTYLE, exMask, exStyle); }
  HWND GetEditControl()
    { return (HWND)SendMessage(CBEM_GETEDITCONTROL, 0, 0); }
};

}}

#endif