// Windows/Control/PropertyPage.h

#pragma once

#ifndef __WINDOWS_CONTROL_PROPERTYPAGE_H
#define __WINDOWS_CONTROL_PROPERTYPAGE_H

#include "Windows/Control/Dialog.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

BOOL APIENTRY ProperyPageProcedure(HWND aDialogHWND, UINT aMessage, UINT wParam, LONG lParam);

class CPropertyPage: public CDialog
{
public:
  CPropertyPage(HWND aWindowNew = NULL): CDialog(aWindowNew){};
  
  void Changed() { PropSheet_Changed(GetParent(), HWND(*this)); }
  void UnChanged() { PropSheet_UnChanged(GetParent(), HWND(*this)); }

  virtual bool OnNotify(UINT aControlID, LPNMHDR lParam);

  virtual bool OnKillActive() { return false; } // false = OK
  virtual bool OnKillActive(const PSHNOTIFY *aPSHNOTIFY) { return OnKillActive(); }
  virtual LONG OnSetActive() { return false; } // false = OK
  virtual LONG OnSetActive(const PSHNOTIFY *aPSHNOTIFY) { return OnKillActive(); }
  virtual LONG OnApply() { return PSNRET_NOERROR; }
  virtual LONG OnApply(const PSHNOTIFY *aPSHNOTIFY) { return OnApply(); }
  virtual void OnNotifyHelp() { }
  virtual void OnNotifyHelp(const PSHNOTIFY *aPSHNOTIFY) { OnNotifyHelp(); }
  virtual void OnReset() { }
  virtual void OnReset(const PSHNOTIFY *aPSHNOTIFY) { OnReset(); }
};


}}

#endif
