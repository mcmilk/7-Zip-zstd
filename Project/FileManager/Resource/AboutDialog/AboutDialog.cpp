// AboutDialog.cpp

#include "StdAfx.h"

#include "resource.h"
#include "AboutDialog.h"
#include "Common/String.h"
#include "../../HelpUtils.h"
#include "../../LangUtils.h"

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_ABOUT_STATIC_REGISTER_INFO, 0x01000103 },
  { IDC_ABOUT_BUTTON_REGISTER, 0x01000105 },
  { IDOK,      0x02000702 }
};

static LPCTSTR kHomePageURL = TEXT("http://www.7-zip.org/");
static LPCTSTR kRegisterRegNowURL = TEXT("https://www.regnow.com/softsell/nph-softsell.cgi?item=2521-1&vreferrer=program");

static LPCTSTR kRegisterTopic = _T("gui/7-zipCfg/info.htm");

bool CAboutDialog::OnInit() 
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  return CModalDialog::OnInit();
}
/*
void CAboutDialog::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kRegisterTopic);
}
*/

bool CAboutDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDC_ABOUT_BUTTON_HOMEPAGE:
      ::ShellExecute(NULL, NULL, kHomePageURL, NULL, NULL, SW_SHOWNORMAL);
      break;
    case IDC_ABOUT_BUTTON_REGISTER:
    {
      LCID aLCID = ::GetUserDefaultLCID();
      LPCTSTR aRegisterURL = kRegisterRegNowURL;
      if (aLCID == 0x0419 || aLCID == 0x422 || aLCID == 0x0423)
        aRegisterURL = TEXT("http://www.7-zip.org/ru/donate.html");
      ::ShellExecute(NULL, NULL, aRegisterURL, NULL, NULL, SW_SHOWNORMAL);
      break;
    }
    default:
      return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
  }
  return true;
}
