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
  UINT64 m_Size;
  CComPtr<ISequentialInStream> m_Stream;
public:
  void Init(ISequentialInStream *aStream, UINT64 aStreamSize);
 
BEGIN_COM_MAP(CLimitedSequentialInStream)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CLimitedSequentialInStream)
DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};

#endif