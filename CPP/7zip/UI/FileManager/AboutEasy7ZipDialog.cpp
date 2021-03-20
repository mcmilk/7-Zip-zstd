// AboutEasy7ZipDialog.cpp

#include "StdAfx.h"
#include "AboutEasy7ZipDialog.h"

#include "HelpUtils.h"

#define kHelpTopic "start.htm"

bool CAboutEasy7ZipDialog::OnInit()
{
  NormalizePosition();
  return CModalDialog::OnInit();
}

void CAboutEasy7ZipDialog::OnHelp()
{
  ShowHelpWindow(kHelpTopic);
}

bool CAboutEasy7ZipDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  LPCTSTR url;
  switch(buttonID)
  {
    case IDC_ABOUT_BUTTON_EASY7ZIP_HOMEPAGE: url = TEXT("http://www.e7z.org/"); break;
    default:
      return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
  }

  #ifdef UNDER_CE
  SHELLEXECUTEINFO s;
  memset(&s, 0, sizeof(s));
  s.cbSize = sizeof(s);
  s.lpFile = url;
  ::ShellExecuteEx(&s);
  #else
  ::ShellExecute(NULL, NULL, url, NULL, NULL, SW_SHOWNORMAL);
  #endif

  return true;
}
