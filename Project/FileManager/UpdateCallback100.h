// UpdateCallback.h

#pragma once

#ifndef __UPDATECALLBACK100_H
#define __UPDATECALLBACK100_H

#include "Interface/CryptoInterface.h"

#include "../Archiver/Common/FolderArchiveInterface.h"
#include "Resource/ProgressDialog/ProgressDialog.h"
// #include "resource.h"

#include "Common/String.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

class CUpdateCallback100Imp: 
  public IFolderArchiveUpdateCallback,
  public ICryptoGetTextPassword2,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CUpdateCallback100Imp)
  COM_INTERFACE_ENTRY(IFolderArchiveUpdateCallback)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CUpdateCallback100Imp)

DECLARE_NO_REGISTRY()

  // IProgress

  STDMETHOD(SetTotal)(UINT64 size);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IUpdateCallBack
  STDMETHOD(CompressOperation)(const wchar_t *name);
  STDMETHOD(DeleteOperation)(const wchar_t *name);
  STDMETHOD(OperationResult)(INT32 operationResult);

  STDMETHOD(CryptoGetTextPassword2)(INT32 *passwordIsDefined, BSTR *password);
private:
  bool _passwordIsDefined;
  UString _password;

public:
  CProgressDialog ProgressDialog;
  HWND _parentWindow;
  void Init(HWND parentWindow, 
      bool passwordIsDefined, const UString &password)
  {
    _passwordIsDefined = passwordIsDefined;
    _password = password;
    _parentWindow = parentWindow;

  }
  void StartProgressDialog(const CSysString &title)
  {
    ProgressDialog.Create(title, _parentWindow);
  }

};



#endif
