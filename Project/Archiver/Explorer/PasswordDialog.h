// PasswordDialog.h

#pragma once

#ifndef __PASSWORDDIALOG_H
#define __PASSWORDDIALOG_H

#include "Windows/Control/Dialog.h"
#include "resource.h"

class CPasswordDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CDialogChildControl m_PasswordControl;
  virtual void OnOK();
  virtual bool OnInit();
public:
  CSysString m_Password;
  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_PASSWORD), aWndParent); }
};

#endif
