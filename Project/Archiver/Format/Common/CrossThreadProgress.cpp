// CrossThreadProgress.cpp

#include "StdAfx.h"

#include "CrossThreadProgress.h"

STDMETHODIMP CCrossThreadProgress::SetRatioInfo(const UINT64 *anInSize, const UINT64 *anOutSize)
{
  m_InSize = anInSize;
  m_OutSize = anOutSize;
  m_ProgressEvent.Set();
  m_WaitEvent.Lock();
  return m_Result;
}

