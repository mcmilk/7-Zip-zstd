// PasswordDialog.h

#pragma once

#ifndef __PASSWORDDIALOG_H
#define __PASSWORDDIALOG_H

#include "Windows/Control/Dialog.h"
#include "resource.h"

class CPasswordDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CDialogChildControl _passwordControl;
  virtual void OnOK();
  virtual bool OnInit();
public:
  CSysString _password;
  INT_PTR Create(HWND parentWindow = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_PASSWORD), parentWindow); }
};

#endif
