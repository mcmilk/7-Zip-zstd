// CrossThreadProgress.h

#pragma once

#ifndef __CROSSTHREADPROGRESS_H
#define __CROSSTHREADPROGRESS_H

#include "../../../Compress/Interface/CompressInterface.h"
#include "Windows/Synchronization.h"

class CCrossThreadProgress: 
  public ICompressProgressInfo,
  public CComObjectRoot
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

BEGIN_COM_MAP(CCrossThreadProgress)
  COM_INTERFACE_ENTRY(ICompressProgressInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCrossThreadProgress)

DECLARE_NO_REGISTRY()

  STDMETHOD(SetRatioInfo)(const UINT64 *inSize, const UINT64 *outSize);
};

#endif
