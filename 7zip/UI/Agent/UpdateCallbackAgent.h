// UpdateCallbackAgent.h

#ifndef __UPDATECALLBACKAGENT_H
#define __UPDATECALLBACKAGENT_H

#include "../Common/UpdateCallback.h"
#include "IFolderArchive.h"

class CUpdateCallbackAgent: public IUpdateCallbackUI
{
  virtual HRESULT SetTotal(UINT64 size);
  virtual HRESULT SetCompleted(const UINT64 *completeValue);
  virtual HRESULT CheckBreak();
  virtual HRESULT Finilize();
  virtual HRESULT GetStream(const wchar_t *name, bool isAnti);
  virtual HRESULT OpenFileError(const wchar_t *name, DWORD systemError);
  virtual HRESULT SetOperationResult(INT32 operationResult);
  virtual HRESULT CryptoGetTextPassword2(INT32 *passwordIsDefined, BSTR *password);
  CMyComPtr<ICryptoGetTextPassword2> _cryptoGetTextPassword;
public:
  CMyComPtr<IFolderArchiveUpdateCallback> Callback;
};

#endif
