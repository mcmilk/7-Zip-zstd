// AboutDialog.cpp

#include "StdAfx.h"

#include "AboutDialogRes.h"
#include "AboutDialog.h"
#include "HelpUtils.h"
#include "LangUtils.h"

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_ABOUT_STATIC_REGISTER_INFO, 0x01000103 },
  { IDC_ABOUT_BUTTON_SUPPORT, 0x01000104 },
  { IDC_ABOUT_BUTTON_REGISTER, 0x01000105 },
  { IDOK, 0x02000702 }
};

#define MY_HOME_PAGE TEXT("http://www.7-zip.org/")

static LPCTSTR kHomePageURL     = MY_HOME_PAGE;
static LPCTSTR kRegisterPageURL = MY_HOME_PAGE TEXT("register.html");
static LPCTSTR kSupportPageURL  = MY_HOME_PAGE TEXT("support.html");

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

static void MyShellExecute(LPCTSTR url)
{
  ::ShellExecute(NULL, NULL, url, NULL, NULL, SW_SHOWNORMAL);
}

bool CAboutDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDC_ABOUT_BUTTON_HOMEPAGE:
      ::MyShellExecute(kHomePageURL);
      break;
    case IDC_ABOUT_BUTTON_REGISTER:
      ::MyShellExecute(kRegisterPageURL);
      break;
    case IDC_ABOUT_BUTTON_SUPPORT:
      ::MyShellExecute(kSupportPageURL);
      break;
    default:
      return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
  }
  return true;
}
