// MenuPage.h
 
#ifndef ZIP7_INC_MENU_PAGE_H
#define ZIP7_INC_MENU_PAGE_H

#include "../../../Windows/Control/PropertyPage.h"
#include "../../../Windows/Control/ComboBox.h"
#include "../../../Windows/Control/ListView.h"

struct CShellDll
{
  FString Path;
  bool wasChanged;
  bool prevValue;
  unsigned ctrl;
  UInt32 wow;

  CShellDll(): wasChanged (false), prevValue(false), ctrl(0), wow(0) {}
};

class CMenuPage: public NWindows::NControl::CPropertyPage
{
  bool _initMode;

  bool _cascaded_Changed;
  bool _menuIcons_Changed;
  bool _elimDup_Changed;
  bool _writeZone_Changed;
  bool _flags_Changed;

  void Clear_MenuChanged()
  {
    _cascaded_Changed = false;
    _menuIcons_Changed = false;
    _elimDup_Changed = false;
    _writeZone_Changed = false;
    _flags_Changed = false;
  }
  
  #ifndef UNDER_CE
  CShellDll _dlls[2];
  #endif
  
  NWindows::NControl::CListView _listView;
  NWindows::NControl::CComboBox _zoneCombo;

  virtual bool OnInit() Z7_override;
  virtual void OnNotifyHelp() Z7_override;
  virtual bool OnNotify(UINT controlID, LPNMHDR lParam) Z7_override;
  virtual LONG OnApply() Z7_override;
  virtual bool OnButtonClicked(unsigned buttonID, HWND buttonHWND) Z7_override;
  virtual bool OnCommand(unsigned code, unsigned itemID, LPARAM param) Z7_override;

  bool OnItemChanged(const NMLISTVIEW* info);
};

#endif
