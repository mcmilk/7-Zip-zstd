// AboutEasy7ZipDialog.h

#ifndef __ABOUT_EASY7ZIP_DIALOG_H
#define __ABOUT_EASY7ZIP_DIALOG_H

#include "AboutEasy7ZipDialogRes.h"
#include "../../../Windows/Control/Dialog.h"

class CAboutEasy7ZipDialog: public NWindows::NControl::CModalDialog
{
public:
  virtual bool OnInit();
  virtual void OnHelp();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  INT_PTR Create(HWND wndParent = 0) { return CModalDialog::Create(IDD_ABOUT_EASY_7ZIP, wndParent); }
};

#endif
