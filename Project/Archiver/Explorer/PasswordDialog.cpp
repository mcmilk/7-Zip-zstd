// PasswordDialog.cpp

#include "StdAfx.h"
#include "PasswordDialog.h"

#ifdef LANG        
#include "../Common/LangUtils.h"
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
  m_PasswordControl.Init(*this, IDC_EXTRACT_EDIT_PASSWORD);
  m_PasswordControl.SetText(_T(""));
  return CModalDialog::OnInit();
}

void CPasswordDialog::OnOK()
{
  m_PasswordControl.GetText(m_Password);
  CModalDialog::OnOK();
}
