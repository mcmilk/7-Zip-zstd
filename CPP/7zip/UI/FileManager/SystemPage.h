// SystemPage.h
 
#ifndef __SYSTEM_PAGE_H
#define __SYSTEM_PAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/ListView.h"

#include "FilePlugins.h"

class CSystemPage: public NWindows::NControl::CPropertyPage
{
  bool _initMode;
  CExtDatabase _extDatabase;

  NWindows::NControl::CListView _listViewExt;
  NWindows::NControl::CListView _listViewPlugins;

  void SetMainPluginText(int itemIndex, int indexInDatabase);

  int GetSelectedExtIndex();
  void RefreshPluginsList(int selectIndex);
  void MovePlugin(bool upDirection);
  void UpdateDatabase();
  void SelectAll();

public:
  bool WasChanged;
  CSystemPage(): WasChanged(false) {}
  virtual bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnNotify(UINT controlID, LPNMHDR lParam);
  virtual bool OnItemChanged(const NMLISTVIEW *info);

  virtual LONG OnApply();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  bool OnPluginsKeyDown(LPNMLVKEYDOWN keyDownInfo);
};

#endif
