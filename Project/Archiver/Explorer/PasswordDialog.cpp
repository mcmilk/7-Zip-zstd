// PasswordDialog.cpp

#include "StdAfx.h"
#include "PasswordDialog.h"

bool CPasswordDialog::OnInit() 
{
  m_PasswordControl.Init(*this, IDC_EXTRACT_EDIT_PASSWORD);
  m_PasswordControl.SetText(_T(""));
  return CModalDialog::OnInit();
}

void CPasswordDialog::OnOK()
{
  m_PasswordControl.GetText(m_Password);
  CModalDialog::OnOK();
}
