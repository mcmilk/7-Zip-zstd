// OpenCallback.h

#pragma once

#ifndef __OPENCALLBACK_H
#define __OPENCALLBACK_H

#include "Common/String.h"

#include "../Format/Common/ArchiveInterface.h"
#include "Interface/CryptoInterface.h"

class COpenCallbackImp: 
  public IArchiveOpenCallback,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(COpenCallbackImp)
  COM_INTERFACE_ENTRY(IArchiveOpenCallback)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COpenCallbackImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(SetTotal)(const UINT64 *files, const UINT64 *bytes);
  STDMETHOD(SetCompleted)(const UINT64 *files, const UINT64 *bytes);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);
public:
  bool PasswordIsDefined;
  UString Password;
  COpenCallbackImp(): PasswordIsDefined(false) {}
};

#endif
