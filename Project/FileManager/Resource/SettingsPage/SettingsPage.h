// SettingsPage.h
 
#pragma once

#ifndef __SETTINGSPAGE_H
#define __SETTINGSPAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/Edit.h"

class CSettingsPage: public NWindows::NControl::CPropertyPage
{
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnCommand(int code, int itemID, LPARAM param);
  virtual LONG OnApply();
};

#endif
