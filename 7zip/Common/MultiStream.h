// MultiStream.h

#pragma once

#ifndef __MULTISTREAM_H
#define __MULTISTREAM_H

#include "../../Windows/Synchronization.h"
#include "../../Common/MyCom.h"
#include "../IStream.h"

class CLockedInStream
{
  CMyComPtr<IInStream> _stream;
  NWindows::NSynchronization::CCriticalSection _criticalSection;
public:
  void Init(IInStream *stream)
    { _stream = stream; }
  HRESULT Read(UINT64 startPos, void *data, UINT32 size, UINT32 *processedSize);
  HRESULT ReadPart(UINT64 startPos, void *data, UINT32 size, UINT32 *processedSize);
};

class CLockedSequentialInStreamImp: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  CLockedInStream *_lockedInStream;
  UINT64 _pos;
public:
  void Init(CLockedInStream *lockedInStream, UINT64 startPos)
  {
    _lockedInStream = lockedInStream;
    _pos = startPos;
  }

  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
};

#endif
