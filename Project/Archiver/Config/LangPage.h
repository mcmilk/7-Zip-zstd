// LangPage.h
 
#pragma once

#ifndef __LANGPAGE_H
#define __LANGPAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/ComboBox.h"

#include "../Common/ZipRegistryMain.h"

class CLangPage: public NWindows::NControl::CPropertyPage
{
  NWindows::NControl::CComboBox m_Lang;
  CSysStringVector m_Paths; 
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnCommand(int aCode, int anItemID, LPARAM lParam);
  virtual LONG OnApply();
};

#endif
