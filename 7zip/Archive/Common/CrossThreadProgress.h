// CrossThreadProgress.h

#pragma once

#ifndef __CROSSTHREADPROGRESS_H
#define __CROSSTHREADPROGRESS_H

#include "../../ICoder.h"
#include "../../../Windows/Synchronization.h"
#include "../../../Common/MyCom.h"

class CCrossThreadProgress: 
  public ICompressProgressInfo,
  public CMyUnknownImp
{
public:
  const UINT64 *InSize;
  const UINT64 *OutSize;
  HRESULT Result;
  NWindows::NSynchronization::CAutoResetEvent ProgressEvent;
  NWindows::NSynchronization::CAutoResetEvent WaitEvent;
  void Init()
  {
    ProgressEvent.Reset();
    WaitEvent.Reset();
  }

  MY_UNKNOWN_IMP

  STDMETHOD(SetRatioInfo)(const UINT64 *inSize, const UINT64 *outSize);
};

#endif
