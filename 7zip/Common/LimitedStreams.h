// LimitedStreams.h

#pragma once

#ifndef __LIMITEDSTREAMS_H
#define __LIMITEDSTREAMS_H

#include "../../Common/MyCom.h"
#include "../IStream.h"

class CLimitedSequentialInStream: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  UINT64 _size;
  CMyComPtr<ISequentialInStream> _stream;
public:
  void Init(ISequentialInStream *stream, UINT64 streamSize);
 
  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
};

#endif
