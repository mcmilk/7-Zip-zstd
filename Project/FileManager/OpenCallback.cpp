// OpenCallback.cpp

#include "StdAfx.h"

#include "OpenCallback.h"

#include "Common/StringConvert.h"
#include "Resource/PasswordDialog/PasswordDialog.h"

STDMETHODIMP COpenArchiveCallback::SetTotal(const UINT64 *numFiles, const UINT64 *numBytes)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UINT64 *numFiles, const UINT64 *numBytes)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetTotal(const UINT64 total)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::SetCompleted(const UINT64 *completed)
{
  return S_OK;
}

STDMETHODIMP COpenArchiveCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!_passwordIsDefined)
  {
    CPasswordDialog dialog;
   
    if (dialog.Create(_parentWindow) == IDCANCEL)
      return E_ABORT;

    _password = GetUnicodeString((LPCTSTR)dialog._password);
    _passwordIsDefined = true;
  }
  CComBSTR tempName = _password;
  *password = tempName.Detach();

  return S_OK;
}
