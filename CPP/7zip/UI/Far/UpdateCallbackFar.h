// UpdateCallbackFar.h

#ifndef ZIP7_INC_UPDATE_CALLBACK_FAR_H
#define ZIP7_INC_UPDATE_CALLBACK_FAR_H

#include "../../../Common/MyCom.h"

#include "../../IPassword.h"

#include "../Agent/IFolderArchive.h"

#include "ProgressBox.h"

Z7_CLASS_IMP_COM_6(
  CUpdateCallback100Imp
  , IFolderArchiveUpdateCallback
  , IFolderArchiveUpdateCallback2
  , IFolderScanProgress
  , ICryptoGetTextPassword2
  , ICryptoGetTextPassword
  , IArchiveOpenCallback
)
  Z7_IFACE_COM7_IMP(IProgress)

  // CMyComPtr<IInFolderArchive> _archiveHandler;
  CProgressBox *_percent;
  // UInt64 _total;
public:
  bool PasswordIsDefined;
  UString Password;

  CUpdateCallback100Imp()
    // : _total(0)
    {}
  void Init(/* IInFolderArchive *archiveHandler, */ CProgressBox *progressBox)
  {
    // _archiveHandler = archiveHandler;
    _percent = progressBox;
    PasswordIsDefined = false;
    Password.Empty();
  }
};

#endif
