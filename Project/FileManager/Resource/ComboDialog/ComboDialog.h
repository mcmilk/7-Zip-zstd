// ComboDialog.h

#pragma once

#ifndef __COMBODIALOG_H
#define __COMBODIALOG_H

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ComboBox.h"
#include "resource.h"

class CComboDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CComboBox _comboBox;
  virtual void OnOK();
  virtual bool OnInit();
public:
  // bool Sorted;
  CSysString Title;
  CSysString Static;
  CSysString Value;
  CSysStringVector Strings;

  // CComboDialog(): Sorted(false) {};
  INT_PTR Create(HWND parentWindow = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_COMBO), parentWindow); }
};

#endif
