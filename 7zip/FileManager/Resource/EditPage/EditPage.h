// EditPage.h
 
#ifndef __EDITPAGE_H
#define __EDITPAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/Edit.h"

class CEditPage: public NWindows::NControl::CPropertyPage
{
  NWindows::NControl::CEdit _editorEdit;
  void OnSetEditorButton();
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnCommand(int code, int itemID, LPARAM param);
  virtual LONG OnApply();
  virtual bool OnButtonClicked(int aButtonID, HWND aButtonHWND);
};

#endif
