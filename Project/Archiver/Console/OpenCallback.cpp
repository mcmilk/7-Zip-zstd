// OpenCallback.cpp

#include "StdAfx.h"

#include "OpenCallback.h"

#include "Common/StdOutStream.h"
#include "Common/StdInStream.h"
#include "Common/StringConvert.h"

#include "ConsoleCloseUtils.h"

STDMETHODIMP COpenCallbackImp::SetTotal(const UINT64 *files, const UINT64 *bytes)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP COpenCallbackImp::SetCompleted(const UINT64 *files, const UINT64 *bytes)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}
  
STDMETHODIMP COpenCallbackImp::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    g_StdOut << "\nEnter password:";
    AString oemPassword = g_StdIn.ScanStringUntilNewLine();
    Password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
    PasswordIsDefined = true;
  }
  CComBSTR temp = Password;
  *password = temp.Detach();
  return S_OK;
}
  
