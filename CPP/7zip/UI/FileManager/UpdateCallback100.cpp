// UpdateCallback100.cpp

#include "StdAfx.h"

#include "MessagesDialog.h"
#include "PasswordDialog.h"
#include "UpdateCallback100.h"

CUpdateCallback100Imp::~CUpdateCallback100Imp()
{
  if (ShowMessages && !Messages.IsEmpty())
  {
    CMessagesDialog messagesDialog;
    messagesDialog.Messages = &Messages;
    messagesDialog.Create(_parentWindow);
  }
}

void CUpdateCallback100Imp::AddErrorMessage(LPCWSTR message)
{
  Messages.Add(message);
}

STDMETHODIMP CUpdateCallback100Imp::SetNumFiles(UInt64 numFiles)
{
  ProgressDialog.ProgressSynch.SetNumFilesTotal(numFiles);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetTotal(UInt64 size)
{
  ProgressDialog.ProgressSynch.SetProgress(size, 0);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetCompleted(const UInt64 *completeValue)
{
  RINOK(ProgressDialog.ProgressSynch.ProcessStopAndPause());
  if (completeValue != NULL)
    ProgressDialog.ProgressSynch.SetPos(*completeValue);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  ProgressDialog.ProgressSynch.SetRatioInfo(inSize, outSize);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CompressOperation(const wchar_t *name)
{
  ProgressDialog.ProgressSynch.SetCurrentFileName(name);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::DeleteOperation(const wchar_t *name)
{
  ProgressDialog.ProgressSynch.SetCurrentFileName(name);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::OperationResult(Int32 /* operationResult */)
{
  NumFiles++;
  ProgressDialog.ProgressSynch.SetNumFilesCur(NumFiles);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::UpdateErrorMessage(const wchar_t *message)
{
  AddErrorMessage(message);
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
  return ProgressDialog.ProgressSynch.ProcessStopAndPause();
}

STDMETHODIMP CUpdateCallback100Imp::CryptoGetTextPassword(BSTR *password)
{
  *password = NULL;
  if (!_passwordIsDefined)
  {
    CPasswordDialog dialog;
    if (dialog.Create(_parentWindow) == IDCANCEL)
      return E_ABORT;
    _password = dialog.Password;
    _passwordIsDefined = true;
  }
  return StringToBstr(_password, password);
}
