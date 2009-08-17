// PasswordDialog.cpp

#include "StdAfx.h"

#include "PasswordDialog.h"

#ifdef LANG
#include "LangUtils.h"
#endif

#ifdef LANG
static CIDLangPair kIDLangPairs[] =
{
  { IDC_STATIC_PASSWORD_HEADER, 0x02000B01 },
  { IDC_CHECK_PASSWORD_SHOW, 0x02000B02 },
  { IDOK, 0x02000702 },
  { IDCANCEL, 0x02000710 }
};
#endif


bool CPasswordDialog::OnInit()
{
  #ifdef LANG
  LangSetWindowText(HWND(*this), 0x02000B00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _passwordControl.Attach(GetItem(IDC_EDIT_PASSWORD));
  _passwordControl.SetText(Password);
  _passwordControl.SetPasswordChar(TEXT('*'));
  return CModalDialog::OnInit();
}

bool CPasswordDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  if (buttonID == IDC_CHECK_PASSWORD_SHOW)
  {
    _passwordControl.SetPasswordChar(IsButtonCheckedBool(IDC_CHECK_PASSWORD_SHOW) ? 0: TEXT('*'));
    UString password;
    _passwordControl.GetText(password);
    _passwordControl.SetText(password);
    return true;
  }
  return CDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CPasswordDialog::OnOK()
{
  _passwordControl.GetText(Password);
  CModalDialog::OnOK();
}
