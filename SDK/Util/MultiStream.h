// MultiStream.h

#pragma once

#ifndef __MULTISTREAM_H
#define __MULTISTREAM_H

#include "Interface/IInOutStreams.h"
#include "Windows/Synchronization.h"

class CLockedInStream
{
  CComPtr<IInStream> _stream;
  NWindows::NSynchronization::CCriticalSection _criticalSection;
public:
  void Init(IInStream *stream)
    { _stream = stream; }
  HRESULT Read(UINT64 startPos, void *data, UINT32 size, UINT32 *processedSize);
  HRESULT ReadPart(UINT64 startPos, void *data, UINT32 size, UINT32 *processedSize);
};

class CLockedSequentialInStreamImp: 
  public ISequentialInStream,
  public CComObjectRoot
{
  CLockedInStream *_lockedInStream;
  UINT64 _pos;
public:
  void Init(CLockedInStream *lockedInStream, UINT64 startPos)
  {
    _lockedInStream = lockedInStream;
    _pos = startPos;
  }

BEGIN_COM_MAP(CLockedSequentialInStreamImp)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CLockedSequentialInStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
};



#endif