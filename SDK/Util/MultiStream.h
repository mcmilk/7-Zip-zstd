// MultiStream.h

#pragma once

#ifndef __MULTISTREAM_H
#define __MULTISTREAM_H

#include "Interface/IInOutStreams.h"
#include "Windows/Synchronization.h"

class CLockedInStream
{
  CComPtr<IInStream> m_Stream;
  NWindows::NSynchronization::CCriticalSection m_CriticalSection;
public:
  void Init(IInStream *aStream)
    { m_Stream = aStream; }
  HRESULT Read(UINT64 aStartPos, void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  HRESULT ReadPart(UINT64 aStartPos, void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};

class CLockedSequentialInStreamImp: 
  public ISequentialInStream,
  public CComObjectRoot
{
  CLockedInStream *m_LockedInStream;
  UINT64 m_Pos;
public:
  void Init(CLockedInStream *aLockedInStream, UINT64 aStartPos)
  {
    m_LockedInStream = aLockedInStream;
    m_Pos = aStartPos;
  }

BEGIN_COM_MAP(CLockedSequentialInStreamImp)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CLockedSequentialInStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};



#endif