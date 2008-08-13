// UpdateCallback.h

#ifndef __UPDATECALLBACK100_H
#define __UPDATECALLBACK100_H

#include "Common/MyCom.h"

#include "../Agent/IFolderArchive.h"

#include "ProgressBox.h"

class CUpdateCallback100Imp:
  public IFolderArchiveUpdateCallback,
  public CMyUnknownImp
{
  // CMyComPtr<IInFolderArchive> _archiveHandler;
  CProgressBox *_progressBox;
  UInt64 _total;

public:
  MY_UNKNOWN_IMP

  INTERFACE_IProgress(;)
  INTERFACE_IFolderArchiveUpdateCallback(;)

  CUpdateCallback100Imp(): _total(0) {}
  void Init(/* IInFolderArchive *archiveHandler, */ CProgressBox *progressBox)
  {
    // _archiveHandler = archiveHandler;
    _progressBox = progressBox;
  }
};



#endif
