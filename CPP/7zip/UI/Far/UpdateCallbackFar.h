// UpdateCallbackFar.h

#ifndef ZIP7_INC_UPDATE_CALLBACK_FAR_H
#define ZIP7_INC_UPDATE_CALLBACK_FAR_H

#include "../../../Common/MyCom.h"

#include "../../IPassword.h"

#include "../Agent/IFolderArchive.h"

#include "ProgressBox.h"

Z7_CLASS_IMP_COM_7(
  CUpdateCallback100Imp
  , IFolderArchiveUpdateCallback
  , IFolderArchiveUpdateCallback2
  , IFolderArchiveUpdateCallback_MoveArc
  , IFolderScanProgress
  , ICryptoGetTextPassword2
  , ICryptoGetTextPassword
  , IArchiveOpenCallback
)
  Z7_IFACE_COM7_IMP(IProgress)

  // CMyComPtr<IInFolderArchive> _archiveHandler;
  CProgressBox *_percent;
  // UInt64 _total;

  HRESULT MoveArc_UpdateStatus();

private:
  UInt64 _arcMoving_total;
  UInt64 _arcMoving_current;
  UInt64 _arcMoving_percents;
  // Int32  _arcMoving_updateMode;

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
    _arcMoving_total = 0;
    _arcMoving_current = 0;
    _arcMoving_percents = 0;
    //  _arcMoving_updateMode = 0;
  }
};

#endif
