// InfoPage.cpp

#include "StdAfx.h"
#include "resource.h"
#include "InfoPage.h"
#include "Common/String.h"
#include "../Common/HelpUtils.h"
#include "../Common/LangUtils.h"

// static const char *kEMail = "mailto:support@7-zip.org";
// static const char *kRegisterHomePage = "http://www.7-zip.org/register.html";

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_STATIC_INFO_REGISTER_INFO, 0x01000103 },
  { IDC_BUTTON_INFO_REGISTER,      0x01000105 }
};

static LPCTSTR kHomePageURL = TEXT("http://www.7-zip.org/");
static LPCTSTR kRegisterRegNowURL = TEXT("https://www.regnow.com/softsell/nph-softsell.cgi?item=2521-1&vreferrer=program");

static LPCTSTR kRegisterTopic = _T("gui/7-zipCfg/info.htm");

bool CInfoPage::OnInit() 
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  return CPropertyPage::OnInit();
}

void CInfoPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kRegisterTopic);
}

bool CInfoPage::OnButtonClicked(int aButtonID, HWND aButtonHWND)
{
  switch(aButtonID)
  {
    case IDC_BUTTON_INFO_HOMEPAGE:
      ::ShellExecute(NULL, NULL, kHomePageURL, NULL, NULL, SW_SHOWNORMAL);
      break;
    case IDC_BUTTON_INFO_REGISTER:
    {
      LCID aLCID = ::GetUserDefaultLCID();
      LPCTSTR aRegisterURL = kRegisterRegNowURL;
      if (aLCID == 0x0419 || aLCID == 0x422 || aLCID == 0x0423)
        aRegisterURL = TEXT("http://www.7-zip.org/ru/donate.html");
      ::ShellExecute(NULL, NULL, aRegisterURL, NULL, NULL, SW_SHOWNORMAL);
      break;
    }
    default:
      return CPropertyPage::OnButtonClicked(aButtonID, aButtonHWND);
  }
  return true;
}
