// CrossThreadProgress.cpp

#include "StdAfx.h"

#include "CrossThreadProgress.h"

STDMETHODIMP CCrossThreadProgress::SetRatioInfo(const UINT64 *inSize, const UINT64 *outSize)
{
  InSize = inSize;
  OutSize = outSize;
  ProgressEvent.Set();
  WaitEvent.Lock();
  return Result;
}

