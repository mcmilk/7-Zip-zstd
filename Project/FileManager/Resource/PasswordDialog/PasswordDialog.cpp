// PasswordDialog.cpp

#include "StdAfx.h"
#include "PasswordDialog.h"

#ifdef LANG        
#include "../../LangUtils.h"
#endif

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDC_STATIC_PASSWORD_HEADER, 0x02000B01 }
};
#endif


bool CPasswordDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000B00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _passwordControl.Init(*this, IDC_EDIT_PASSWORD);
  _passwordControl.SetText(_T(""));
  return CModalDialog::OnInit();
}

void CPasswordDialog::OnOK()
{
  _passwordControl.GetText(_password);
  CModalDialog::OnOK();
}
