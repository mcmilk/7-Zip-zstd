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
  CComPtr<ICompressProgressInfo> _progress;
  bool _inStartValueIsAssigned;
  bool _outStartValueIsAssigned;
  UINT64 _inStartValue;
  UINT64 _outStartValue;
public:
  void Init(ICompressProgressInfo *progress, 
      const UINT64 *inStartValue, const UINT64 *outStartValue);

BEGIN_COM_MAP(CLocalCompressProgressInfo)
  COM_INTERFACE_ENTRY(ICompressProgressInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CLocalCompressProgressInfo)

DECLARE_NO_REGISTRY()

  STDMETHOD(SetRatioInfo)(const UINT64 *inSize, const UINT64 *outSize);
};


///////////////////////////////////////////
// CLocalProgress
    
class CLocalProgress: 
  public ICompressProgressInfo,
  public CComObjectRoot
{
  CComPtr<IProgress> _progress;
  bool _inSizeIsMain;
public:
  void Init(IProgress *progress, bool inSizeIsMain);

BEGIN_COM_MAP(CLocalProgress)
  COM_INTERFACE_ENTRY(ICompressProgressInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CLocalProgress)

DECLARE_NO_REGISTRY()

  STDMETHOD(SetRatioInfo)(const UINT64 *inSize, const UINT64 *outSize);
};


#endif
