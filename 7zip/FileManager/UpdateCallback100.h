// UpdateCallback100.h

#ifndef __UPDATE_CALLBACK100_H
#define __UPDATE_CALLBACK100_H

#include "Common/MyCom.h"
#include "Common/String.h"

#include "../UI/Agent/IFolderArchive.h"
#include "Resource/ProgressDialog2/ProgressDialog.h"
#include "../IPassword.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

class CUpdateCallback100Imp: 
  public IFolderArchiveUpdateCallback,
  public ICryptoGetTextPassword2,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(
    IFolderArchiveUpdateCallback, 
    ICryptoGetTextPassword2)

  // IProgress

  STDMETHOD(SetTotal)(UINT64 size);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IUpdateCallBack
  STDMETHOD(CompressOperation)(const wchar_t *name);
  STDMETHOD(DeleteOperation)(const wchar_t *name);
  STDMETHOD(OperationResult)(INT32 operationResult);
  STDMETHOD(UpdateErrorMessage)(const wchar_t *message);

  STDMETHOD(CryptoGetTextPassword2)(INT32 *passwordIsDefined, BSTR *password);
private:
  bool _passwordIsDefined;
  UString _password;

  void AddErrorMessage(LPCWSTR message);
  CSysStringVector Messages;
public:
  ~CUpdateCallback100Imp();
  CProgressDialog ProgressDialog;
  HWND _parentWindow;
  void Init(HWND parentWindow, 
      bool passwordIsDefined, const UString &password)
  {
    _passwordIsDefined = passwordIsDefined;
    _password = password;
    _parentWindow = parentWindow;

  }
  void StartProgressDialog(const UString &title)
  {
    ProgressDialog.Create(title, _parentWindow);
  }

};

#endif
