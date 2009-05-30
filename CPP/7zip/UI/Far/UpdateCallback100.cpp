// UpdateCallback.h

#include "StdAfx.h"

#include "UpdateCallback100.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "FarUtils.h"

using namespace NFar;

STDMETHODIMP CUpdateCallback100Imp::SetNumFiles(UInt64 /* numFiles */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetTotal(UInt64 size)
{
  _total = size;
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetCompleted(const UInt64 *completeValue)
{
  if (WasEscPressed())
    return E_ABORT;
  if (_progressBox != 0)
    _progressBox->Progress(&_total, completeValue, AString());
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CompressOperation(const wchar_t* /* name */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::DeleteOperation(const wchar_t* /* name */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::OperationResult(Int32 /* opRes */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::UpdateErrorMessage(const wchar_t *message)
{
  if (g_StartupInfo.ShowMessage(UnicodeStringToMultiByte(message, CP_OEMCP)) == -1)
    return E_ABORT;
  return S_OK;
}

extern HRESULT GetPassword(UString &password);

STDMETHODIMP CUpdateCallback100Imp::CryptoGetTextPassword(BSTR *password)
{
  *password = NULL;
  if (!m_PasswordIsDefined)
  {
    RINOK(GetPassword(m_Password));
    m_PasswordIsDefined = true;
  }
  return StringToBstr(m_Password, password);
}
