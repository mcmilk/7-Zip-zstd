// UpdateCallback100.h

#ifndef __UPDATE_CALLBACK100_H
#define __UPDATE_CALLBACK100_H

#include "Common/MyCom.h"
#include "Common/MyString.h"

#include "../Agent/IFolderArchive.h"
#include "ProgressDialog2.h"
#include "../../IPassword.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

class CUpdateCallback100Imp: 
  public IFolderArchiveUpdateCallback,
  public ICryptoGetTextPassword2,
  public ICompressProgressInfo,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP3(
    IFolderArchiveUpdateCallback, 
    ICryptoGetTextPassword2,
    ICompressProgressInfo)

  // IProgress

  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);
  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);

  // IUpdateCallBack
  STDMETHOD(CompressOperation)(const wchar_t *name);
  STDMETHOD(DeleteOperation)(const wchar_t *name);
  STDMETHOD(OperationResult)(Int32 operationResult);
  STDMETHOD(UpdateErrorMessage)(const wchar_t *message);
  STDMETHOD(SetNumFiles)(UInt64 numFiles);

  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);
private:
  bool _passwordIsDefined;
  UString _password;

  void AddErrorMessage(LPCWSTR message);
  bool ShowMessages;

public:
  CUpdateCallback100Imp(): ShowMessages(true) {}
  ~CUpdateCallback100Imp();
  CProgressDialog ProgressDialog;
  HWND _parentWindow;
  UStringVector Messages;
  UInt64 NumFolders;
  UInt64 NumFiles;

  void Init(HWND parentWindow, 
      bool passwordIsDefined, const UString &password)
  {
    _passwordIsDefined = passwordIsDefined;
    _password = password;
    _parentWindow = parentWindow;
    NumFolders = NumFiles = 0;
  }
  void StartProgressDialog(const UString &title)
  {
    ProgressDialog.Create(title, _parentWindow);
  }

};

#endif
