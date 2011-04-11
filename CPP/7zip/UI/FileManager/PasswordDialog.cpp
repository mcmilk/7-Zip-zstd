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

void CPasswordDialog::ReadControls()
{
  _passwordControl.GetText(Password);
  ShowPassword = IsButtonCheckedBool(IDC_CHECK_PASSWORD_SHOW);
}

void CPasswordDialog::SetTextSpec()
{
  _passwordControl.SetPasswordChar(ShowPassword ? 0: TEXT('*'));
  _passwordControl.SetText(Password);
}

bool CPasswordDialog::OnInit()
{
  #ifdef LANG
  LangSetWindowText(HWND(*this), 0x02000B00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _passwordControl.Attach(GetItem(IDC_EDIT_PASSWORD));
  CheckButton(IDC_CHECK_PASSWORD_SHOW, ShowPassword);
  SetTextSpec();
  return CModalDialog::OnInit();
}

bool CPasswordDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  if (buttonID == IDC_CHECK_PASSWORD_SHOW)
  {
    ReadControls();
    SetTextSpec();
    return true;
  }
  return CDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CPasswordDialog::OnOK()
{
  ReadControls();
  CModalDialog::OnOK();
}
