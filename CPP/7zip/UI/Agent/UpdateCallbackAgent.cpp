// UpdateCallbackAgent.h

#include "StdAfx.h"

#include "Windows/Error.h"
#include "Common/IntToString.h"

#include "UpdateCallbackAgent.h"

using namespace NWindows;

void CUpdateCallbackAgent::SetCallback(IFolderArchiveUpdateCallback *callback)
{
  Callback = callback;
  _compressProgress.Release();
  if (Callback)
    Callback.QueryInterface(IID_ICompressProgressInfo, &_compressProgress);
}

HRESULT CUpdateCallbackAgent::SetNumFiles(UInt64 numFiles)
{
  if (Callback)
    return Callback->SetNumFiles(numFiles);
  return S_OK;
}


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

HRESULT CUpdateCallbackAgent::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  if (_compressProgress)
    return _compressProgress->SetRatioInfo(inSize, outSize);
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
  // if (systemError == ERROR_SHARING_VIOLATION)
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

HRESULT CUpdateCallbackAgent::GetStream(const wchar_t *name, bool /* isAnti */)
{
  if (Callback)
    return Callback->CompressOperation(name);
  return S_OK;
}

HRESULT CUpdateCallbackAgent::SetOperationResult(Int32 operationResult)
{
  if (Callback)
    return Callback->OperationResult(operationResult);
  return S_OK;
}

HRESULT CUpdateCallbackAgent::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
  *password = NULL;
  *passwordIsDefined = BoolToInt(false);
  if (!_cryptoGetTextPassword)
  {
    if (!Callback)
      return S_OK;
    Callback.QueryInterface(IID_ICryptoGetTextPassword2, &_cryptoGetTextPassword);
    if (!_cryptoGetTextPassword)
      return S_OK;
  }
  return _cryptoGetTextPassword->CryptoGetTextPassword2(passwordIsDefined, password);
}

HRESULT CUpdateCallbackAgent::CryptoGetTextPassword(BSTR *password)
{
  *password = NULL;
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  Callback.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
  if (!getTextPassword)
    return E_NOTIMPL;
  return getTextPassword->CryptoGetTextPassword(password);
}

/*
HRESULT CUpdateCallbackAgent::ShowDeleteFile(const wchar_t *name)
{
  return Callback->DeleteOperation(name);
}
*/
