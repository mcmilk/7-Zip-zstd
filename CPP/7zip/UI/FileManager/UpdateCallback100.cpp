// UpdateCallback100.cpp

#include "StdAfx.h"

#include "PasswordDialog.h"
#include "UpdateCallback100.h"

STDMETHODIMP CUpdateCallback100Imp::SetNumFiles(UInt64 numFiles)
{
  ProgressDialog->Sync.Set_NumFilesTotal(numFiles);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetTotal(UInt64 size)
{
  ProgressDialog->Sync.Set_NumBytesTotal(size);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetCompleted(const UInt64 *completed)
{
  return ProgressDialog->Sync.Set_NumBytesCur(completed);
}

STDMETHODIMP CUpdateCallback100Imp::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  ProgressDialog->Sync.Set_Ratio(inSize, outSize);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CompressOperation(const wchar_t *name)
{
  ProgressDialog->Sync.Set_FilePath(name);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::DeleteOperation(const wchar_t *name)
{
  ProgressDialog->Sync.Set_FilePath(name);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::OperationResult(Int32 /* operationResult */)
{
  ProgressDialog->Sync.Set_NumFilesCur(++_numFiles);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::UpdateErrorMessage(const wchar_t *message)
{
  ProgressDialog->Sync.AddError_Message(message);
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
  return ProgressDialog->Sync.CheckStop();
}

STDMETHODIMP CUpdateCallback100Imp::CryptoGetTextPassword(BSTR *password)
{
  *password = NULL;
  if (!_passwordIsDefined)
  {
    CPasswordDialog dialog;
    ProgressDialog->WaitCreating();
    if (dialog.Create(*ProgressDialog) != IDOK)
      return E_ABORT;
    _password = dialog.Password;
    _passwordIsDefined = true;
  }
  return StringToBstr(_password, password);
}
