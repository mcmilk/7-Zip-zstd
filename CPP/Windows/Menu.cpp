// Windows/Menu.cpp

#include "StdAfx.h"

#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif
#include "Windows/Menu.h"

#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NWindows {

static void ConvertItemToSysForm(const CMenuItem &item, MENUITEMINFOW &si)
{
  ZeroMemory(&si, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = item.fMask;
  si.fType = item.fType;
  si.fState = item.fState;
  si.wID = item.wID;
  si.hSubMenu = item.hSubMenu;
  si.hbmpChecked = item.hbmpChecked;
  si.hbmpUnchecked = item.hbmpUnchecked;
  si.dwItemData = item.dwItemData;
}

#ifndef _UNICODE
static void ConvertItemToSysForm(const CMenuItem &item, MENUITEMINFOA &si)
{
  ZeroMemory(&si, sizeof(si));
  si.cbSize = sizeof(si);
  si.fMask = item.fMask;
  si.fType = item.fType;
  si.fState = item.fState;
  si.wID = item.wID;
  si.hSubMenu = item.hSubMenu;
  si.hbmpChecked = item.hbmpChecked;
  si.hbmpUnchecked = item.hbmpUnchecked;
  si.dwItemData = item.dwItemData;
}
#endif

static void ConvertItemToMyForm(const MENUITEMINFOW &si, CMenuItem &item)
{
  item.fMask = si.fMask;
  item.fType = si.fType;
  item.fState = si.fState;
  item.wID = si.wID;
  item.hSubMenu = si.hSubMenu;
  item.hbmpChecked = si.hbmpChecked;
  item.hbmpUnchecked = si.hbmpUnchecked;
  item.dwItemData = si.dwItemData;
}

#ifndef _UNICODE
static void ConvertItemToMyForm(const MENUITEMINFOA &si, CMenuItem &item)
{
  item.fMask = si.fMask;
  item.fType = si.fType;
  item.fState = si.fState;
  item.wID = si.wID;
  item.hSubMenu = si.hSubMenu;
  item.hbmpChecked = si.hbmpChecked;
  item.hbmpUnchecked = si.hbmpUnchecked;
  item.dwItemData = si.dwItemData;
}
#endif

bool CMenu::GetItem(UINT itemIndex, bool byPosition, CMenuItem &item)
{
  const UINT kMaxSize = 512;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    CHAR s[kMaxSize + 1];
    MENUITEMINFOA si;
    ConvertItemToSysForm(item, si);
    if (item.IsString())
    {
      si.cch = kMaxSize;
      si.dwTypeData = s;
    }
    if (GetItemInfo(itemIndex, byPosition, &si))
    {
      ConvertItemToMyForm(si, item);
      if (item.IsString())
        item.StringValue = GetUnicodeString(s);
      return true;
    }
  }
  else
  #endif
  {
    wchar_t s[kMaxSize + 1];
    MENUITEMINFOW si;
    ConvertItemToSysForm(item, si);
    if (item.IsString())
    {
      si.cch = kMaxSize;
      si.dwTypeData = s;
    }
    if (GetItemInfo(itemIndex, byPosition, &si))
    {
      ConvertItemToMyForm(si, item);
      if (item.IsString())
        item.StringValue = s;
      return true;
    }
  }
  return false;
}

bool CMenu::SetItem(UINT itemIndex, bool byPosition, const CMenuItem &item)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    MENUITEMINFOA si;
    ConvertItemToSysForm(item, si);
    AString s;
    if (item.IsString())
    {
      s = GetSystemString(item.StringValue);
      si.dwTypeData = (LPTSTR)(LPCTSTR)s;
    }
    return SetItemInfo(itemIndex, byPosition, &si);
  }
  else
  #endif
  {
    MENUITEMINFOW si;
    ConvertItemToSysForm(item, si);
    if (item.IsString())
      si.dwTypeData = (LPWSTR)(LPCWSTR)item.StringValue;
    return SetItemInfo(itemIndex, byPosition, &si);
  }
}

bool CMenu::InsertItem(UINT itemIndex, bool byPosition, const CMenuItem &item)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    MENUITEMINFOA si;
    ConvertItemToSysForm(item, si);
    AString s;
    if (item.IsString())
    {
      s = GetSystemString(item.StringValue);
      si.dwTypeData = (LPTSTR)(LPCTSTR)s;
    }
    return InsertItem(itemIndex, byPosition, &si);
  }
  else
  #endif
  {
    MENUITEMINFOW si;
    ConvertItemToSysForm(item, si);
    if (item.IsString())
      si.dwTypeData = (LPWSTR)(LPCWSTR)item.StringValue;
    #ifdef UNDER_CE
    UINT flags = (item.fType & MFT_SEPARATOR) ? MF_SEPARATOR : MF_STRING;
    UINT id = item.wID;
    if ((item.fMask & MIIM_SUBMENU) != 0)
    {
      flags |= MF_POPUP;
      id = (UINT)item.hSubMenu;
    }
    if (!Insert(itemIndex, flags | (byPosition ? MF_BYPOSITION : MF_BYCOMMAND), id, item.StringValue))
      return false;
    return SetItemInfo(itemIndex, byPosition, &si);
    #else
    return InsertItem(itemIndex, byPosition, &si);
    #endif
  }
}

#ifndef _UNICODE
bool CMenu::AppendItem(UINT flags, UINT_PTR newItemID, LPCWSTR newItem)
{
  if (g_IsNT)
    return BOOLToBool(::AppendMenuW(_menu, flags, newItemID, newItem));
  else
    return AppendItem(flags, newItemID, GetSystemString(newItem));
}
#endif

}
