// OpenCallbackConsole.cpp

#include "StdAfx.h"

#include "OpenCallbackConsole.h"

#include "ConsoleClose.h"
#include "UserInputUtils.h"

HRESULT COpenCallbackConsole::CheckBreak()
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

HRESULT COpenCallbackConsole::SetTotal(const UInt64 *, const UInt64 *)
{
  return CheckBreak();
}

HRESULT COpenCallbackConsole::SetCompleted(const UInt64 *, const UInt64 *)
{
  return CheckBreak();
}
 
HRESULT COpenCallbackConsole::CryptoGetTextPassword(BSTR *password)
{
  PasswordWasAsked = true;
  RINOK(CheckBreak());
  if (!PasswordIsDefined)
  {
    Password = GetPassword(OutStream); 
    PasswordIsDefined = true;
  }
  CMyComBSTR temp(Password);
  *password = temp.Detach();
  return S_OK;
}

HRESULT COpenCallbackConsole::GetPasswordIfAny(UString &password)
{
  if (PasswordIsDefined)
    password = Password;
  return S_OK;
}

bool COpenCallbackConsole::WasPasswordAsked()
{
  return PasswordWasAsked;
}

void COpenCallbackConsole::ClearPasswordWasAskedFlag()
{
  PasswordWasAsked = false;
}

  
