// ProgressUtils.h

#pragma once

#ifndef __PROGRESSUTILS_H
#define __PROGRESSUTILS_H

#include "Interface/ICoder.h"
#include "Interface/IProgress.h"


class CLocalCompressProgressInfo: 
  public ICompressProgressInfo,
  public CComObjectRoot
{
  CComPtr<ICompressProgressInfo> m_Progress;
  bool m_InStartValueIsAssigned;
  bool m_OutStartValueIsAssigned;
  UINT64 m_InStartValue;
  UINT64 m_OutStartValue;
public:
  void Init(ICompressProgressInfo *aProgress, 
      const UINT64 *anInStartValue, const UINT64 *anOutStartValue);

BEGIN_COM_MAP(CLocalCompressProgressInfo)
  COM_INTERFACE_ENTRY(ICompressProgressInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CLocalCompressProgressInfo)

DECLARE_NO_REGISTRY()

  STDMETHOD(SetRatioInfo)(const UINT64 *anInSize, const UINT64 *anOutSize);
};


///////////////////////////////////////////
// CLocalProgress
    
class CLocalProgress: 
  public ICompressProgressInfo,
  public CComObjectRoot
{
  CComPtr<IProgress> m_Progress;
  bool m_InSizeIsMain;
public:
  void Init(IProgress *aProgress, bool anInSizeIsMain);

BEGIN_COM_MAP(CLocalProgress)
  COM_INTERFACE_ENTRY(ICompressProgressInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CLocalProgress)

DECLARE_NO_REGISTRY()

  STDMETHOD(SetRatioInfo)(const UINT64 *anInSize, const UINT64 *anOutSize);
};


#endif
