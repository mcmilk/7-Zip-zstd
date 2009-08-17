// UpdateCallback100.h

#ifndef __UPDATE_CALLBACK100_H
#define __UPDATE_CALLBACK100_H

#include "Common/MyCom.h"

#include "../../IPassword.h"

#include "../Agent/IFolderArchive.h"

#include "ProgressDialog2.h"

class CUpdateCallback100Imp:
  public IFolderArchiveUpdateCallback,
  public ICryptoGetTextPassword2,
  public ICryptoGetTextPassword,
  public IArchiveOpenCallback,
  public ICompressProgressInfo,
  public CMyUnknownImp
{
  bool _passwordIsDefined;
  UString _password;
  UInt64 _numFiles;
public:
  CProgressDialog *ProgressDialog;

  CUpdateCallback100Imp(): ProgressDialog(0) {}

  MY_UNKNOWN_IMP5(
    IFolderArchiveUpdateCallback,
    ICryptoGetTextPassword2,
    ICryptoGetTextPassword,
    IArchiveOpenCallback,
    ICompressProgressInfo)

  INTERFACE_IProgress(;)
  INTERFACE_IArchiveOpenCallback(;)
  INTERFACE_IFolderArchiveUpdateCallback(;)

  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);

  STDMETHOD(CryptoGetTextPassword)(BSTR *password);
  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);

  void Init(bool passwordIsDefined, const UString &password)
  {
    _passwordIsDefined = passwordIsDefined;
    _password = password;
    _numFiles = 0;
  }
};

#endif
