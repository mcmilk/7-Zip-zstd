// InfoPage.h
 
#pragma once

#ifndef __INFOPAGE_H
#define __INFOPAGE_H

#include "Windows/Control/PropertyPage.h"

class CInfoPage: public NWindows::NControl::CPropertyPage
{
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnButtonClicked(int aButtonID, HWND aButtonHWND);
};

#endif
