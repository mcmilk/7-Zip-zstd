// UpdateCallbackFar.h

#ifndef __UPDATE_CALLBACK_FAR_H
#define __UPDATE_CALLBACK_FAR_H

#include "../../../Common/MyCom.h"

#include "../../IPassword.h"

#include "../Agent/IFolderArchive.h"

#include "ProgressBox.h"

class CUpdateCallback100Imp:
  public IFolderArchiveUpdateCallback,
  public IFolderArchiveUpdateCallback2,
  public IFolderScanProgress,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
  // CMyComPtr<IInFolderArchive> _archiveHandler;
  CProgressBox *_percent;
  UInt64 _total;
  bool m_PasswordIsDefined;
  UString m_Password;

public:
  MY_UNKNOWN_IMP4(
      IFolderArchiveUpdateCallback,
      IFolderArchiveUpdateCallback2,
      IFolderScanProgress,
      ICryptoGetTextPassword)

  INTERFACE_IProgress(;)
  INTERFACE_IFolderArchiveUpdateCallback(;)
  INTERFACE_IFolderArchiveUpdateCallback2(;)
  INTERFACE_IFolderScanProgress(;)

  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  CUpdateCallback100Imp(): _total(0) {}
  void Init(/* IInFolderArchive *archiveHandler, */ CProgressBox *progressBox)
  {
    // _archiveHandler = archiveHandler;
    _percent = progressBox;
    m_PasswordIsDefined = false;
  }
};

#endif
