// InfoPage.cpp

#include "StdAfx.h"
#include "resource.h"
#include "InfoPage.h"
#include "Common/String.h"
#include "../Common/HelpUtils.h"

// static const char *kEMail = "mailto:support@7-zip.org";
// static const char *kRegisterHomePage = "http://www.7-zip.org/register.html";

static LPCTSTR kHomePageURL = "http://www.7-zip.org/";
static LPCTSTR kRegisterRegNowURL = "https://www.regnow.com/softsell/nph-softsell.cgi?item=2521-1&vreferrer=program";

static LPCTSTR kRegisterTopic = _T("gui/7-zipCfg/register.htm");

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
      ::ShellExecute(NULL, NULL, kRegisterRegNowURL, NULL, NULL, SW_SHOWNORMAL);
      break;
    default:
      return CPropertyPage::OnButtonClicked(aButtonID, aButtonHWND);
  }
  return true;
}
