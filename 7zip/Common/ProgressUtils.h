// ProgressUtils.h

// #pragma once

#ifndef __PROGRESSUTILS_H
#define __PROGRESSUTILS_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"
#include "../IProgress.h"

class CLocalCompressProgressInfo: 
  public ICompressProgressInfo,
  public CMyUnknownImp
{
  CMyComPtr<ICompressProgressInfo> _progress;
  bool _inStartValueIsAssigned;
  bool _outStartValueIsAssigned;
  UINT64 _inStartValue;
  UINT64 _outStartValue;
public:
  void Init(ICompressProgressInfo *progress, 
      const UINT64 *inStartValue, const UINT64 *outStartValue);

  MY_UNKNOWN_IMP

  STDMETHOD(SetRatioInfo)(const UINT64 *inSize, const UINT64 *outSize);
};


///////////////////////////////////////////
// CLocalProgress
    
class CLocalProgress: 
  public ICompressProgressInfo,
  public CMyUnknownImp
{
  CMyComPtr<IProgress> _progress;
  bool _inSizeIsMain;
public:
  void Init(IProgress *progress, bool inSizeIsMain);

  MY_UNKNOWN_IMP

  STDMETHOD(SetRatioInfo)(const UINT64 *inSize, const UINT64 *outSize);
};


#endif
