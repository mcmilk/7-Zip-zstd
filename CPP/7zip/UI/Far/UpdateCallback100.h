// UpdateCallback.h

#ifndef __UPDATE_CALLBACK_H
#define __UPDATE_CALLBACK_H

#include "Common/MyCom.h"

#include "../Agent/IFolderArchive.h"

#include "../../IPassword.h"

#include "ProgressBox.h"

class CUpdateCallback100Imp:
  public IFolderArchiveUpdateCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
  // CMyComPtr<IInFolderArchive> _archiveHandler;
  CProgressBox *_progressBox;
  UInt64 _total;
  bool m_PasswordIsDefined;
  UString m_Password;

public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

  INTERFACE_IProgress(;)
  INTERFACE_IFolderArchiveUpdateCallback(;)
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  CUpdateCallback100Imp(): _total(0) {}
  void Init(/* IInFolderArchive *archiveHandler, */ CProgressBox *progressBox)
  {
    // _archiveHandler = archiveHandler;
    _progressBox = progressBox;
    m_PasswordIsDefined = false;
  }
};



#endif
