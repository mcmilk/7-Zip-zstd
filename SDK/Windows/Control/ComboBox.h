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
  int AddString(LPCTSTR aString)
    { return SendMessage(CB_ADDSTRING, 0, (LPARAM)aString); }
  int SetCurSel(int anIndex)
    { return SendMessage(CB_SETCURSEL, anIndex, 0); }
  int GetCurSel()
    { return SendMessage(CB_GETCURSEL, 0, 0); }
  int GetCount()
    { return SendMessage(CB_GETCOUNT, 0, 0); }
  
  int GetLBTextLen(int anIndex)
    { return SendMessage(CB_GETLBTEXTLEN, anIndex, 0); }
  int GetLBText(int anIndex, LPTSTR aString)
    { return SendMessage(CB_GETLBTEXT, anIndex, (LPARAM)aString); }
  int GetLBText(int anIndex, CSysString &aString);

  int SetItemData(int anIndex, LPARAM lParam)
    { return SendMessage(CB_SETITEMDATA, anIndex, lParam); }
  int GetItemData(int anIndex)
    { return SendMessage(CB_GETITEMDATA, anIndex, 0); }
};

class CComboBoxEx: public CWindow
{
public:
  int DeleteItem(int anIndex)
    { SendMessage(CBEM_DELETEITEM, anIndex, 0); }
  int InsertItem(COMBOBOXEXITEM *anItem)
    { return SendMessage(CBEM_INSERTITEM, 0, (LPARAM)anItem); }
};

}}

#endif