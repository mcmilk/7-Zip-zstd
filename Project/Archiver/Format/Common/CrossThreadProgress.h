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
  const UINT64 *m_InSize;
  const UINT64 *m_OutSize;
  HRESULT m_Result;

  NWindows::NSynchronization::CAutoResetEvent m_ProgressEvent;
  NWindows::NSynchronization::CAutoResetEvent m_WaitEvent;
  void Init()
  {
    m_ProgressEvent.Reset();
    m_WaitEvent.Reset();
  }

BEGIN_COM_MAP(CCrossThreadProgress)
  COM_INTERFACE_ENTRY(ICompressProgressInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCrossThreadProgress)

DECLARE_NO_REGISTRY()

  STDMETHOD(SetRatioInfo)(const UINT64 *anInSize, const UINT64 *anOutSize);
};

#endif
