// LimitedStreams.h

#pragma once

#ifndef __LIMITEDSTREAMS_H
#define __LIMITEDSTREAMS_H

//////////////////////////////////////////////////
// CLimitedSequentialInStream

#include "IInOutStreams.h"

class CLimitedSequentialInStream: 
  public ISequentialInStream,
  public CComObjectRoot
{
  UINT64 _size;
  CComPtr<ISequentialInStream> _stream;
public:
  void Init(ISequentialInStream *stream, UINT64 streamSize);
 
BEGIN_COM_MAP(CLimitedSequentialInStream)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CLimitedSequentialInStream)
DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
};

#endif