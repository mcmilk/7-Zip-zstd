// SettingsPage.cpp

#include "StdAfx.h"

#include "../../../Common/StringConvert.h"

#ifndef UNDER_CE
#include "../../../Windows/MemoryLock.h"
#endif

#include "HelpUtils.h"
#include "LangUtils.h"
#include "RegistryUtils.h"
#include "SettingsPage.h"

#include "SettingsPageRes.h"

using namespace NWindows;

static const UInt32 kLangIDs[] =
{
  IDX_SETTINGS_SHOW_DOTS,
  IDX_SETTINGS_SHOW_REAL_FILE_ICONS,
  IDX_SETTINGS_SHOW_SYSTEM_MENU,
  IDX_SETTINGS_FULL_ROW,
  IDX_SETTINGS_SHOW_GRID,
  IDX_SETTINGS_SINGLE_CLICK,
  IDX_SETTINGS_ALTERNATIVE_SELECTION,
  IDX_SETTINGS_LARGE_PAGES
};

static LPCWSTR kEditTopic = L"FM/options.htm#settings";

extern bool IsLargePageSupported();

bool CSettingsPage::OnInit()
{
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));

  CheckButton(IDX_SETTINGS_SHOW_DOTS, ReadShowDots());
  CheckButton(IDX_SETTINGS_SHOW_SYSTEM_MENU, Read_ShowSystemMenu());
  CheckButton(IDX_SETTINGS_SHOW_REAL_FILE_ICONS, ReadShowRealFileIcons());
  CheckButton(IDX_SETTINGS_FULL_ROW, ReadFullRow());
  CheckButton(IDX_SETTINGS_SHOW_GRID, ReadShowGrid());
  CheckButton(IDX_SETTINGS_ALTERNATIVE_SELECTION, ReadAlternativeSelection());
  if (IsLargePageSupported())
    CheckButton(IDX_SETTINGS_LARGE_PAGES, ReadLockMemoryEnable());
  else
    EnableItem(IDX_SETTINGS_LARGE_PAGES, false);
  CheckButton(IDX_SETTINGS_SINGLE_CLICK, ReadSingleClick());
  // CheckButton(IDX_SETTINGS_UNDERLINE, ReadUnderline());

  // EnableSubItems();

  return CPropertyPage::OnInit();
}

/*
void CSettingsPage::EnableSubItems()
{
  EnableItem(IDX_SETTINGS_UNDERLINE, IsButtonCheckedBool(IDX_SETTINGS_SINGLE_CLICK));
}
*/

LONG CSettingsPage::OnApply()
{
  SaveShowDots(IsButtonCheckedBool(IDX_SETTINGS_SHOW_DOTS));
  Save_ShowSystemMenu(IsButtonCheckedBool(IDX_SETTINGS_SHOW_SYSTEM_MENU));
  SaveShowRealFileIcons(IsButtonCheckedBool(IDX_SETTINGS_SHOW_REAL_FILE_ICONS));

  SaveFullRow(IsButtonCheckedBool(IDX_SETTINGS_FULL_ROW));
  SaveShowGrid(IsButtonCheckedBool(IDX_SETTINGS_SHOW_GRID));
  SaveAlternativeSelection(IsButtonCheckedBool(IDX_SETTINGS_ALTERNATIVE_SELECTION));
  #ifndef UNDER_CE
  if (IsLargePageSupported())
  {
    bool enable = IsButtonCheckedBool(IDX_SETTINGS_LARGE_PAGES);
    NSecurity::EnablePrivilege_LockMemory(enable);
    SaveLockMemoryEnable(enable);
  }
  #endif
  
  SaveSingleClick(IsButtonCheckedBool(IDX_SETTINGS_SINGLE_CLICK));
  // SaveUnderline(IsButtonCheckedBool(IDX_SETTINGS_UNDERLINE));

  return PSNRET_NOERROR;
}

void CSettingsPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kEditTopic); // change it
}

bool CSettingsPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDX_SETTINGS_SINGLE_CLICK:
    /*
      EnableSubItems();
      break;
    */
    case IDX_SETTINGS_SHOW_DOTS:
    case IDX_SETTINGS_SHOW_SYSTEM_MENU:
    case IDX_SETTINGS_SHOW_REAL_FILE_ICONS:
    case IDX_SETTINGS_FULL_ROW:
    case IDX_SETTINGS_SHOW_GRID:
    case IDX_SETTINGS_ALTERNATIVE_SELECTION:
    case IDX_SETTINGS_LARGE_PAGES:
      Changed();
      return true;
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
}
