// UpdateCallback.h

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "UpdateCallback100.h"
// #include "Windows/ProcessMessages.h"
// #include "Resource/PasswordDialog/PasswordDialog.h"

#include "Common/Defs.h"

using namespace NWindows;

STDMETHODIMP CUpdateCallback100Imp::SetTotal(UINT64 size)
{
  /*
  if (m_ThreadID != GetCurrentThreadId())
    return S_OK;
  */
  // m_Total = size;
  // m_ProgressDialog.Timer(PDTIMER_RESET);

  _progressDialog._progressSynch.SetProgress(size, 0);

  _appTitle.SetRange(size);
  _appTitle.SetPos(0);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetCompleted(const UINT64 *completeValue)
{
  /*
  if (m_ThreadID != GetCurrentThreadId())
    return S_OK;
  */
  // ProcessMessages(m_ProgressDialog);
  // if(m_ProgressDialog.WasProcessStopped())
  // if(m_ProgressDialog.HasUserCancelled())
  if(_progressDialog._progressSynch.GetStopped())
    return E_ABORT;
  if (completeValue != NULL)
  {
    _progressDialog._progressSynch.SetPos(*completeValue);
    // m_ProgressDialog.SetPos(*completeValue);
    // m_ProgressDialog.SetProgress(*completeValue, m_Total);
    _appTitle.SetPos(*completeValue);
  }
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CompressOperation(const wchar_t *name)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::DeleteOperation(const wchar_t *name)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::OperationResult(INT32 operationResult)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CryptoGetTextPassword2(INT32 *passwordIsDefined, BSTR *password)
{
  *passwordIsDefined = BoolToInt(_passwordIsDefined);
  if (!_passwordIsDefined)
  {
    return S_OK;
    /*
    CPasswordDialog dialog;
    if (dialog.Create(_parentWindow) == IDCANCEL)
      return E_ABORT;
    _password = GetUnicodeString((LPCTSTR)dialog._password);
    _passwordIsDefined = true;
    */
  }
  *passwordIsDefined = BoolToInt(_passwordIsDefined);
  CComBSTR tempName = _password;
  *password = tempName.Detach();
  return S_OK;
}
