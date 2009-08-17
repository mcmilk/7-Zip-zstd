// MenuPage.h
 
#ifndef __MENU_PAGE_H
#define __MENU_PAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/ListView.h"

#include "../Common/LoadCodecs.h"

class CMenuPage: public NWindows::NControl::CPropertyPage
{
  bool _initMode;
  NWindows::NControl::CListView _listView;

  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnNotify(UINT controlID, LPNMHDR lParam);
  virtual bool OnItemChanged(const NMLISTVIEW *info);
  virtual LONG OnApply();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
public:
};

#endif
