// OpenCallback.h

#pragma once

#ifndef __OPENCALLBACK_H
#define __OPENCALLBACK_H

#include "Common/String.h"
#include "Interface/CryptoInterface.h"

#include "../Archiver/Format/Common/ArchiveInterface.h"

class COpenArchiveCallback: 
  public IArchiveOpenCallback,
  public IProgress,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
  bool _passwordIsDefined;
  UString _password;
  HWND _parentWindow;

public:
BEGIN_COM_MAP(COpenArchiveCallback)
  COM_INTERFACE_ENTRY(IArchiveOpenCallback)
  COM_INTERFACE_ENTRY(IProgress)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COpenArchiveCallback)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 total);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IArchiveOpenCallback
  STDMETHOD(SetTotal)(const UINT64 *numFiles, const UINT64 *numBytes);
  STDMETHOD(SetCompleted)(const UINT64 *numFiles, const UINT64 *numBytes);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  void Init()
  {
    _passwordIsDefined = false;
  }
  void ShowMessage(const UINT64 *completed);
};

#endif
