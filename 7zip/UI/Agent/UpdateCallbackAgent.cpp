// UpdateCallbackAgent.h

#include "StdAfx.h"

#include "Windows/Error.h"

#include "UpdateCallbackAgent.h"

using namespace NWindows;

HRESULT CUpdateCallbackAgent::SetTotal(UINT64 size)
{
  if (Callback)
    return Callback->SetTotal(size);
  return S_OK;
}

HRESULT CUpdateCallbackAgent::SetCompleted(const UINT64 *completeValue)
{
  if (Callback)
    return Callback->SetCompleted(completeValue);
  return S_OK;
}

HRESULT CUpdateCallbackAgent::CheckBreak()
{
  return S_OK;
}

HRESULT CUpdateCallbackAgent::Finilize()
{
  return S_OK;
}

HRESULT CUpdateCallbackAgent::OpenFileError(const wchar_t *name, DWORD systemError)
{
  if (systemError == ERROR_SHARING_VIOLATION)
  {
    if (Callback)
    {
      RINOK(Callback->UpdateErrorMessage(
        UString(L"WARNING: ") + 
        NError::MyFormatMessageW(systemError) + 
        UString(L": ") + 
        UString(name)));
      return S_FALSE;
    }
  }
  // FailedFiles.Add(name);
  return systemError;
}

HRESULT CUpdateCallbackAgent::GetStream(const wchar_t *name, bool isAnti)
{
  if (Callback)
    return Callback->CompressOperation(name);
  return S_OK;
}

HRESULT CUpdateCallbackAgent::SetOperationResult(INT32 operationResult)
{
  if (Callback)
    return Callback->OperationResult(operationResult);
  return S_OK;
}

HRESULT CUpdateCallbackAgent::CryptoGetTextPassword2(INT32 *passwordIsDefined, BSTR *password)
{
  *passwordIsDefined = BoolToInt(false);
  if (!_cryptoGetTextPassword)
  {
    if (!Callback)
      return S_OK;
    HRESULT result = Callback.QueryInterface(
        IID_ICryptoGetTextPassword2, &_cryptoGetTextPassword);
    if (result != S_OK)
      return S_OK;
  }
  return _cryptoGetTextPassword->CryptoGetTextPassword2(passwordIsDefined, password);
}
