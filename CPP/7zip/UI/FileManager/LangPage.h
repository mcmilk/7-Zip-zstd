// LangPage.h
 
#ifndef __LANGPAGE_H
#define __LANGPAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/ComboBox.h"

class CLangPage: public NWindows::NControl::CPropertyPage
{
  NWindows::NControl::CComboBox _langCombo;
  UStringVector _paths; 
public:
  bool _langWasChanged;
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnCommand(int code, int itemID, LPARAM param);
  virtual LONG OnApply();
};

#endif
