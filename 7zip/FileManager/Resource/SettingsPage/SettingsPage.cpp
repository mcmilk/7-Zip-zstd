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
  { IDC_SETTINGS_SHOW_SYSTEM_MENU, 0x03010410},
  { IDC_SETTINGS_FULL_ROW, 0x03010420},
  { IDC_SETTINGS_SHOW_GRID, 0x03010421}
  // { IDC_SETTINGS_SINGLE_CLICK, 0x03010422},
  // { IDC_SETTINGS_UNDERLINE, 0x03010423}
};

static LPCWSTR kEditTopic = L"FM/options.htm#settings";

bool CSettingsPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  CheckButton(IDC_SETTINGS_SHOW_DOTS, ReadShowDots());
  CheckButton(IDC_SETTINGS_SHOW_SYSTEM_MENU, ReadShowSystemMenu());
  CheckButton(IDC_SETTINGS_SHOW_REAL_FILE_ICONS, ReadShowRealFileIcons());

  CheckButton(IDC_SETTINGS_FULL_ROW, ReadFullRow());
  CheckButton(IDC_SETTINGS_SHOW_GRID, ReadShowGrid());
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
  
  // SaveSingleClick(IsButtonCheckedBool(IDC_SETTINGS_SINGLE_CLICK));
  // SaveUnderline(IsButtonCheckedBool(IDC_SETTINGS_UNDERLINE));

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
        itemID == IDC_SETTINGS_SHOW_REAL_FILE_ICONS ||
        itemID == IDC_SETTINGS_FULL_ROW ||
        itemID == IDC_SETTINGS_SHOW_GRID
        /*
        ||
        itemID == IDC_SETTINGS_SINGLE_CLICK || 
        itemID == IDC_SETTINGS_UNDERLINE
        */
      )
    )
  {
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, param);
}

/*
bool CSettingsPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{ 
  switch(buttonID)
  {
    case IDC_SETTINGS_SINGLE_CLICK:
      EnableSubItems();
      break;
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
}
*/
