// UpdateCallback.h

#pragma once

#ifndef __UPDATECALLBACK100_H
#define __UPDATECALLBACK100_H

#include "../Archiver/Common/IArchiveHandler2.h"
#include "Resource/ProgressDialog/ProgressDialog.h"
#include "../Archiver/Format/Common/FormatCryptoInterface.h"
// #include "resource.h"

#include "Common/String.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

#include "AppTitle.h"

class CUpdateCallBack100Imp: 
  public IUpdateCallback100,
  public ICryptoGetTextPassword2,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CUpdateCallBack100Imp)
  COM_INTERFACE_ENTRY(IUpdateCallback100)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CUpdateCallBack100Imp)

DECLARE_NO_REGISTRY()

  // IProfress

  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IUpdateCallBack
  STDMETHOD(CompressOperation)(const wchar_t *aName);
  STDMETHOD(DeleteOperation)(const wchar_t *aName);
  STDMETHOD(OperationResult)(INT32 aOperationResult);

  STDMETHOD(CryptoGetTextPassword2)(INT32 *passwordIsDefined, BSTR *password);
private:
  DWORD m_ThreadID;
  bool _passwordIsDefined;
  UString _password;

public:
  ~CUpdateCallBack100Imp()
  {
    m_ProgressDialog.Destroy();
  }
  CAppTitle _appTitle;
  CProgressDialog m_ProgressDialog;
  HWND _parentWindow;
  void Init(/*IArchiveHandler100 *anArchiveHandler, */ HWND aParentWindow, 
      const CSysString &aTitle, bool passwordIsDefined, const UString &password)
  {
    _passwordIsDefined = passwordIsDefined;
    _password = password;
    _parentWindow = aParentWindow;
    m_ThreadID = GetCurrentThreadId();
    m_ProgressDialog.Create(aParentWindow);
    m_ProgressDialog.SetText(aTitle);
    // LangLoadString(IDS_PROGRESS_COMPRESSING, 0x02000DC0));
    m_ProgressDialog.Show(SW_SHOWNORMAL);
    // m_ArchiveHandler = anArchiveHandler;
  }
};



#endif
