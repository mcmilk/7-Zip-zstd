// ProgressUtils.h

#include "StdAfx.h"

#include "ProgressUtils.h"

void CLocalCompressProgressInfo::Init(ICompressProgressInfo *progress,
    const UINT64 *inStartValue, const UINT64 *outStartValue)
{
  _progress = progress;
  _inStartValueIsAssigned = (inStartValue != NULL);
  if (_inStartValueIsAssigned)
    _inStartValue = *inStartValue;
  _outStartValueIsAssigned = (outStartValue != NULL);
  if (_outStartValueIsAssigned)
    _outStartValue = *outStartValue;
}

STDMETHODIMP CLocalCompressProgressInfo::SetRatioInfo(
    const UINT64 *inSize, const UINT64 *outSize)
{
  UINT64 inSizeNew, outSizeNew;
  const UINT64 *inSizeNewPointer;
  const UINT64 *outSizeNewPointer;
  if (_inStartValueIsAssigned && inSize != NULL)
  {
    inSizeNew = _inStartValue + (*inSize);
    inSizeNewPointer = &inSizeNew;
  }
  else
    inSizeNewPointer = NULL;

  if (_outStartValueIsAssigned && outSize != NULL)
  {
    outSizeNew = _outStartValue + (*outSize);
    outSizeNewPointer = &outSizeNew;
  }
  else
    outSizeNewPointer = NULL;
  return _progress->SetRatioInfo(inSizeNewPointer, outSizeNewPointer);
}


///////////////////////////////////
// 

void CLocalProgress::Init(IProgress *progress, bool inSizeIsMain)
{
  _progress = progress;
  _inSizeIsMain = inSizeIsMain;
}

STDMETHODIMP CLocalProgress::SetRatioInfo(
    const UINT64 *inSize, const UINT64 *outSize)
{
  return _progress->SetCompleted(_inSizeIsMain ? inSize : outSize);
}

