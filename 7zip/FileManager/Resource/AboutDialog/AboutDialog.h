// AboutDialog.h
 
#ifndef __ABOUTDIALOG_H
#define __ABOUTDIALOG_H

#include "resource.h"
#include "Windows/Control/Dialog.h"

class CAboutDialog: public NWindows::NControl::CModalDialog
{
public:
  virtual bool OnInit();
  virtual void OnHelp();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_ABOUT), aWndParent); }
};

#endif
