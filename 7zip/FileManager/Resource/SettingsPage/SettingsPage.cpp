// SettingsPage.cpp

#include "StdAfx.h"
#include "resource.h"
#include "SettingsPage.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"

#include "../../RegistryUtils.h"
#include "../../HelpUtils.h"
#include "../../LangUtils.h"
#include "../../ProgramLocation.h"

using namespace NWindows;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_SETTINGS_SHOW_DOTS, 0x03010401},
  { IDC_SETTINGS_SHOW_REAL_FILE_ICONS, 0x03010402},
  { IDC_SETTINGS_SHOW_SYSTEM_MENU, 0x03010410}
};

static LPCTSTR kEditTopic = TEXT("FM/options.htm#settings");

bool CSettingsPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  CheckButton(IDC_SETTINGS_SHOW_DOTS, ReadShowDots());
  CheckButton(IDC_SETTINGS_SHOW_SYSTEM_MENU, ReadShowSystemMenu());
  CheckButton(IDC_SETTINGS_SHOW_REAL_FILE_ICONS, ReadShowRealFileIcons());
  return CPropertyPage::OnInit();
}

LONG CSettingsPage::OnApply()
{
  SaveShowDots(IsButtonCheckedBool(IDC_SETTINGS_SHOW_DOTS));
  SaveShowSystemMenu(IsButtonCheckedBool(IDC_SETTINGS_SHOW_SYSTEM_MENU));
  SaveShowRealFileIcons(IsButtonCheckedBool(IDC_SETTINGS_SHOW_REAL_FILE_ICONS));

  return PSNRET_NOERROR;
}

void CSettingsPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kEditTopic); // change it
}


bool CSettingsPage::OnCommand(int code, int itemID, LPARAM param)
{
  if (code == EN_CHANGE && 
      (
        itemID == IDC_SETTINGS_SHOW_DOTS || 
        itemID == IDC_SETTINGS_SHOW_SYSTEM_MENU || 
        itemID == IDC_SETTINGS_SHOW_REAL_FILE_ICONS
      )
    )
  {
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, param);
}


