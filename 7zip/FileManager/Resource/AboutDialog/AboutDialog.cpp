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
// static LPCTSTR kRegisterRegNowURL = TEXT("https://secure.shareit.com/shareit/checkout.html?PRODUCT[104808]=1&languageid=1");
static LPCTSTR kRegisterRegNowURL = TEXT("https://www.regnow.com/softsell/nph-softsell.cgi?item=2521-1&vreferrer=program");

static LPCTSTR kEmailAction = 
  TEXT("mailto:support@7-zip.org?subject=7-Zip");

static LPCWSTR kHelpTopic = L"start.htm";

bool CAboutDialog::OnInit() 
{
  LangSetWindowText(HWND(*this), 0x01000100);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  return CModalDialog::OnInit();
}

void CAboutDialog::OnHelp()
{
  ShowHelpWindow(NULL, kHelpTopic);
}

bool CAboutDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDC_ABOUT_BUTTON_HOMEPAGE:
      ::ShellExecute(NULL, NULL, kHomePageURL, NULL, NULL, SW_SHOWNORMAL);
      break;
    case IDC_ABOUT_BUTTON_REGISTER:
    {
      LPCTSTR registerURL = kRegisterRegNowURL;
      /*
      LCID aLCID = ::GetUserDefaultLCID();
      if (aLCID == 0x0419 || aLCID == 0x422 || aLCID == 0x0423)
        registerURL = TEXT("http://www.7-zip.org/ru/donate.html");
      */
      ::ShellExecute(NULL, NULL, registerURL, NULL, NULL, SW_SHOWNORMAL);
      break;
    }
    case IDC_ABOUT_BUTTON_EMAIL:
    {
      ::ShellExecute(NULL, NULL, kEmailAction, NULL, NULL, SW_SHOWNORMAL);
      break;
    }
    default:
      return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
  }
  return true;
}
