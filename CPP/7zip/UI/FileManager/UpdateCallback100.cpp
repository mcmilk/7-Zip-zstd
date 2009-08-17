// UpdateCallback100.cpp

#include "StdAfx.h"

#include "PasswordDialog.h"
#include "UpdateCallback100.h"

STDMETHODIMP CUpdateCallback100Imp::SetNumFiles(UInt64 numFiles)
{
  ProgressDialog->Sync.SetNumFilesTotal(numFiles);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetTotal(UInt64 size)
{
  ProgressDialog->Sync.SetProgress(size, 0);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetCompleted(const UInt64 *completeValue)
{
  RINOK(ProgressDialog->Sync.ProcessStopAndPause());
  if (completeValue != NULL)
    ProgressDialog->Sync.SetPos(*completeValue);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  ProgressDialog->Sync.SetRatioInfo(inSize, outSize);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CompressOperation(const wchar_t *name)
{
  ProgressDialog->Sync.SetCurrentFileName(name);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::DeleteOperation(const wchar_t *name)
{
  ProgressDialog->Sync.SetCurrentFileName(name);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::OperationResult(Int32 /* operationResult */)
{
  ProgressDialog->Sync.SetNumFilesCur(++_numFiles);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::UpdateErrorMessage(const wchar_t *message)
{
  ProgressDialog->Sync.AddErrorMessage(message);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
  *password = NULL;
  *passwordIsDefined = BoolToInt(_passwordIsDefined);
  if (!_passwordIsDefined)
    return S_OK;
  return StringToBstr(_password, password);
}

STDMETHODIMP CUpdateCallback100Imp::SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
  return ProgressDialog->Sync.ProcessStopAndPause();
}

STDMETHODIMP CUpdateCallback100Imp::CryptoGetTextPassword(BSTR *password)
{
  *password = NULL;
  if (!_passwordIsDefined)
  {
    CPasswordDialog dialog;
    ProgressDialog->WaitCreating();
    if (dialog.Create(*ProgressDialog) == IDCANCEL)
      return E_ABORT;
    _password = dialog.Password;
    _passwordIsDefined = true;
  }
  return StringToBstr(_password, password);
}
