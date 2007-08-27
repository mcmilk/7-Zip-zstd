// SettingsPage.h
 
#ifndef __SETTINGSPAGE_H
#define __SETTINGSPAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/Edit.h"

class CSettingsPage: public NWindows::NControl::CPropertyPage
{
  // void EnableSubItems();
  bool OnButtonClicked(int buttonID, HWND buttonHWND);
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual LONG OnApply();
};

#endif
