// ProgressUtils.h

#include "StdAfx.h"

#include "ProgressUtils.h"

void CLocalCompressProgressInfo::Init(ICompressProgressInfo *aProgress,
    const UINT64 *anInStartValue, const UINT64 *anOutStartValue)
{
  m_Progress = aProgress;
  m_InStartValueIsAssigned = (anInStartValue != NULL);
  if (m_InStartValueIsAssigned)
    m_InStartValue = *anInStartValue;
  m_OutStartValueIsAssigned = (anOutStartValue != NULL);
  if (m_OutStartValueIsAssigned)
    m_OutStartValue = *anOutStartValue;
}

STDMETHODIMP CLocalCompressProgressInfo::SetRatioInfo(
    const UINT64 *anInSize, const UINT64 *anOutSize)
{
  UINT64 anInSizeNew, anOutSizeNew;
  const UINT64 *anInSizeNewPointer;
  const UINT64 *anOutSizeNewPointer;
  if (m_InStartValueIsAssigned && anInSize != NULL)
  {
    anInSizeNew = m_InStartValue + (*anInSize);
    anInSizeNewPointer = &anInSizeNew;
  }
  else
    anInSizeNewPointer = NULL;

  if (m_OutStartValueIsAssigned && anOutSize != NULL)
  {
    anOutSizeNew = m_OutStartValue + (*anOutSize);
    anOutSizeNewPointer = &anOutSizeNew;
  }
  else
    anOutSizeNewPointer = NULL;
  return m_Progress->SetRatioInfo(anInSizeNewPointer, anOutSizeNewPointer);
}


///////////////////////////////////
// 

void CLocalProgress::Init(IProgress *aProgress, bool anInSizeIsMain)
{
  m_Progress = aProgress;
  m_InSizeIsMain = anInSizeIsMain;
}

STDMETHODIMP CLocalProgress::SetRatioInfo(
    const UINT64 *anInSize, const UINT64 *anOutSize)
{
  return m_Progress->SetCompleted(m_InSizeIsMain ? anInSize : anOutSize);
}

