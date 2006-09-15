// Windows/Control/PropertyPage.h

#ifndef __WINDOWS_CONTROL_PROPERTYPAGE_H
#define __WINDOWS_CONTROL_PROPERTYPAGE_H

#include "Windows/Control/Dialog.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

INT_PTR APIENTRY ProperyPageProcedure(HWND dialogHWND, UINT message, WPARAM wParam, LPARAM lParam);

class CPropertyPage: public CDialog
{
public:
  CPropertyPage(HWND window = NULL): CDialog(window){};
  
  void Changed() { PropSheet_Changed(GetParent(), HWND(*this)); }
  void UnChanged() { PropSheet_UnChanged(GetParent(), HWND(*this)); }

  virtual bool OnNotify(UINT controlID, LPNMHDR lParam);

  virtual bool OnKillActive() { return false; } // false = OK
  virtual bool OnKillActive(const PSHNOTIFY * /* aPSHNOTIFY */) { return OnKillActive(); }
  virtual LONG OnSetActive() { return false; } // false = OK
  virtual LONG OnSetActive(const PSHNOTIFY * /* aPSHNOTIFY */) { return OnKillActive(); }
  virtual LONG OnApply() { return PSNRET_NOERROR; }
  virtual LONG OnApply(const PSHNOTIFY * /* aPSHNOTIFY */) { return OnApply(); }
  virtual void OnNotifyHelp() { }
  virtual void OnNotifyHelp(const PSHNOTIFY * /* aPSHNOTIFY */) { OnNotifyHelp(); }
  virtual void OnReset() { }
  virtual void OnReset(const PSHNOTIFY * /* aPSHNOTIFY */) { OnReset(); }
};

struct CPageInfo
{
  CPropertyPage *Page;
  UString Title;
  UINT ID;
};

INT_PTR MyPropertySheet(const CObjectVector<CPageInfo> &pagesInfo, HWND hwndParent, const UString &title);

}}

#endif
