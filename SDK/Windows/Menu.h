// Windows/Resource/Menu.h

#pragma once

#ifndef __WINDOWS_RESOURCE_MENU_H
#define __WINDOWS_RESOURCE_MENU_H

#include "Common/String.h"
#include "Windows/Defs.h"

namespace NWindows {

class CMenu
{
  HMENU m_Menu;
public:
  CMenu(): m_Menu(NULL) {};
  ~CMenu() { Destroy(); }
  operator HMENU() const { return m_Menu; }
  void Attach(HMENU aWindowNew) { m_Menu = aWindowNew; }
  
  HMENU Detach()
  {
    HMENU aMenu = m_Menu;
    m_Menu = NULL;
    return aMenu;
  }
  
  bool Destroy()
  { 
    if (m_Menu == NULL)
      return false;
    return BOOLToBool(::DestroyMenu(Detach()));
  }
  
  bool CreatePopup()
  { 
    m_Menu = ::CreatePopupMenu();
    return (m_Menu != NULL); 
  }
  
  bool AppendItem(UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR aNewItem)
    { return BOOLToBool(::AppendMenu(m_Menu, uFlags, uIDNewItem, aNewItem)); }

  bool InsertItem(UINT aItem, bool aByPosition, LPCMENUITEMINFO anItem)
    { return BOOLToBool(::InsertMenuItem(m_Menu, aItem, 
        BoolToBOOL(aByPosition), anItem)); }

  int Track(UINT uFlags, int x, int y, HWND hWnd)
    { return ::TrackPopupMenuEx(m_Menu, uFlags, x, y, hWnd, NULL); }
};

}

#endif
