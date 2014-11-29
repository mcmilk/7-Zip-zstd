// PluginsPage.h
 
#include "Windows/Control/ListView.h"

#ifndef __PLUGINSPAGE_H
#define __PLUGINSPAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/ComboBox.h"

#include "RegistryPlugins.h"

class CPluginsPage: public NWindows::NControl::CPropertyPage
{
  NWindows::NControl::CListView _listView;
  CObjectVector<CPluginInfo> _plugins;
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  virtual void OnButtonOptions();
  virtual LONG OnApply();
  virtual bool OnNotify(UINT controlID, LPNMHDR lParam);
};

#endif
