// SettingsPage.cpp

#include "StdAfx.h"
#include "SettingsPageRes.h"
#include "SettingsPage.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/MemoryLock.h"

#include "RegistryUtils.h"
#include "HelpUtils.h"
#include "LangUtils.h"
#include "ProgramLocation.h"

using namespace NWindows;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_SETTINGS_SHOW_DOTS, 0x03010401},
  { IDC_SETTINGS_SHOW_REAL_FILE_ICONS, 0x03010402},
  { IDC_SETTINGS_SHOW_SYSTEM_MENU, 0x03010410},
  { IDC_SETTINGS_FULL_ROW, 0x03010420},
  { IDC_SETTINGS_SHOW_GRID, 0x03010421},
  { IDC_SETTINGS_ALTERNATIVE_SELECTION, 0x03010430},
  { IDC_SETTINGS_LARGE_PAGES, 0x03010440}
  // { IDC_SETTINGS_SINGLE_CLICK, 0x03010422},
  // { IDC_SETTINGS_UNDERLINE, 0x03010423}
};

static LPCWSTR kEditTopic = L"FM/options.htm#settings";

extern bool IsLargePageSupported();

bool CSettingsPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  CheckButton(IDC_SETTINGS_SHOW_DOTS, ReadShowDots());
  CheckButton(IDC_SETTINGS_SHOW_SYSTEM_MENU, ReadShowSystemMenu());
  CheckButton(IDC_SETTINGS_SHOW_REAL_FILE_ICONS, ReadShowRealFileIcons());

  CheckButton(IDC_SETTINGS_FULL_ROW, ReadFullRow());
  CheckButton(IDC_SETTINGS_SHOW_GRID, ReadShowGrid());
  CheckButton(IDC_SETTINGS_ALTERNATIVE_SELECTION, ReadAlternativeSelection());
  if (IsLargePageSupported())
    CheckButton(IDC_SETTINGS_LARGE_PAGES, ReadLockMemoryEnable());
  else
    EnableItem(IDC_SETTINGS_LARGE_PAGES, false);
  // CheckButton(IDC_SETTINGS_SINGLE_CLICK, ReadSingleClick());
  // CheckButton(IDC_SETTINGS_UNDERLINE, ReadUnderline());

  // EnableSubItems();

  return CPropertyPage::OnInit();
}

/*
void CSettingsPage::EnableSubItems()
{
  EnableItem(IDC_SETTINGS_UNDERLINE, IsButtonCheckedBool(IDC_SETTINGS_SINGLE_CLICK));
}
*/

LONG CSettingsPage::OnApply()
{
  SaveShowDots(IsButtonCheckedBool(IDC_SETTINGS_SHOW_DOTS));
  SaveShowSystemMenu(IsButtonCheckedBool(IDC_SETTINGS_SHOW_SYSTEM_MENU));
  SaveShowRealFileIcons(IsButtonCheckedBool(IDC_SETTINGS_SHOW_REAL_FILE_ICONS));

  SaveFullRow(IsButtonCheckedBool(IDC_SETTINGS_FULL_ROW));
  SaveShowGrid(IsButtonCheckedBool(IDC_SETTINGS_SHOW_GRID));
  SaveAlternativeSelection(IsButtonCheckedBool(IDC_SETTINGS_ALTERNATIVE_SELECTION));
  if (IsLargePageSupported())
  {
    bool enable = IsButtonCheckedBool(IDC_SETTINGS_LARGE_PAGES);
    NSecurity::EnableLockMemoryPrivilege(enable);
    SaveLockMemoryEnable(enable);
  }
  
  // SaveSingleClick(IsButtonCheckedBool(IDC_SETTINGS_SINGLE_CLICK));
  // SaveUnderline(IsButtonCheckedBool(IDC_SETTINGS_UNDERLINE));

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
    /*
    case IDC_SETTINGS_SINGLE_CLICK:
      EnableSubItems();
      break;
    */
    case IDC_SETTINGS_SHOW_DOTS:
    case IDC_SETTINGS_SHOW_SYSTEM_MENU:
    case IDC_SETTINGS_SHOW_REAL_FILE_ICONS:
    case IDC_SETTINGS_FULL_ROW:
    case IDC_SETTINGS_SHOW_GRID:
    case IDC_SETTINGS_ALTERNATIVE_SELECTION:
    case IDC_SETTINGS_LARGE_PAGES:
      Changed();
      return true;
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
}
