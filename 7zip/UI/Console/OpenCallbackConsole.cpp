// OpenCallbackConsole.cpp

#include "StdAfx.h"

#include "OpenCallbackConsole.h"

#include "Common/StdOutStream.h"
#include "Common/StdInStream.h"
#include "Common/StringConvert.h"

#include "ConsoleClose.h"

HRESULT COpenCallbackConsole::CheckBreak()
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

HRESULT COpenCallbackConsole::SetTotal(const UInt64 *files, const UInt64 *bytes)
{
  return CheckBreak();
}

HRESULT COpenCallbackConsole::SetCompleted(const UInt64 *files, const UInt64 *bytes)
{
  return CheckBreak();
}
 
HRESULT COpenCallbackConsole::CryptoGetTextPassword(BSTR *password)
{
  RINOK(CheckBreak());
  if (!PasswordIsDefined)
  {
    g_StdErr << "\nEnter password:";
    AString oemPassword = g_StdIn.ScanStringUntilNewLine();
    Password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
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

  
