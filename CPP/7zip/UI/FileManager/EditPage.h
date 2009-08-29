// EditPage.h
 
#ifndef __EDIT_PAGE_H
#define __EDIT_PAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/Edit.h"

class CEditPage: public NWindows::NControl::CPropertyPage
{
  NWindows::NControl::CEdit _editor;
  NWindows::NControl::CEdit _diff;
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnCommand(int code, int itemID, LPARAM param);
  virtual LONG OnApply();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
};

#endif
