// StreamObjects2.h

#pragma once

#ifndef __STREAMOBJECTS2_H
#define __STREAMOBJECTS2_H

#include "Common/DynamicBuffer.h"
#include "Interface/IInOutStreams.h"
#include "../../../Compress/Interface/CompressInterface.h"

class CSequentialInStreamSizeCount2: 
  public ISequentialInStream,
  public ICompressGetSubStreamSize,
  public CComObjectRoot
{
  CComPtr<ISequentialInStream> m_Stream;
  CComPtr<ICompressGetSubStreamSize> m_GetSubStreamSize;
  UINT64 m_Size;
public:
  void Init(ISequentialInStream *aStream)
  {
    m_Stream = aStream;
    m_GetSubStreamSize = 0;
    m_Stream.QueryInterface(&m_GetSubStreamSize);
    m_Size = 0;
  }
  UINT64 GetSize() const { return m_Size; }
BEGIN_COM_MAP(CSequentialInStreamSizeCount2)
  COM_INTERFACE_ENTRY(ISequentialInStream)
  COM_INTERFACE_ENTRY(ICompressGetSubStreamSize)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialInStreamSizeCount2)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);

  STDMETHOD(GetSubStreamSize)(UINT64 aSubStream, UINT64 *aValue);
};


#endif