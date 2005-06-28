// UpdateCallback.h

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "UpdateCallback100.h"
// #include "Windows/ProcessMessages.h"
// #include "Resource/PasswordDialog/PasswordDialog.h"
#include "Resource/MessagesDialog/MessagesDialog.h"

#include "Common/Defs.h"

using namespace NWindows;

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

STDMETHODIMP CUpdateCallback100Imp::SetTotal(UINT64 size)
{
  ProgressDialog.ProgressSynch.SetProgress(size, 0);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetCompleted(const UINT64 *completeValue)
{
  while(true)
  {
    if(ProgressDialog.ProgressSynch.GetStopped())
      return E_ABORT;
    if(!ProgressDialog.ProgressSynch.GetPaused())
      break;
    ::Sleep(100);
  }
  if (completeValue != NULL)
    ProgressDialog.ProgressSynch.SetPos(*completeValue);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CompressOperation(const wchar_t *name)
{
  ProgressDialog.ProgressSynch.SetCurrentFileName(name);
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

STDMETHODIMP CUpdateCallback100Imp::UpdateErrorMessage(const wchar_t *message)
{
  AddErrorMessage(message);
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
  CMyComBSTR tempName = _password;
  *password = tempName.Detach();
  return S_OK;
}
