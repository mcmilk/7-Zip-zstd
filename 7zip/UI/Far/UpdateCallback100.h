// UpdateCallback.h

#pragma once

#ifndef __UPDATECALLBACK100_H
#define __UPDATECALLBACK100_H

#include "Common/String.h"
#include "Common/MyCom.h"

#include "../Agent/IFolderArchive.h"

#include "Far/ProgressBox.h"

class CUpdateCallBack100Imp: 
  public IFolderArchiveUpdateCallback,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  // IProfress

  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IUpdateCallBack
  STDMETHOD(CompressOperation)(const wchar_t *aName);
  STDMETHOD(DeleteOperation)(const wchar_t *aName);
  STDMETHOD(OperationResult)(INT32 aOperationResult);

private:
  CMyComPtr<IInFolderArchive> m_ArchiveHandler;
  CProgressBox *m_ProgressBox;
public:
  void Init(IInFolderArchive *anArchiveHandler,
      CProgressBox *aProgressBox)
  {
    m_ArchiveHandler = anArchiveHandler;
    m_ProgressBox = aProgressBox;
  }
};



#endif
