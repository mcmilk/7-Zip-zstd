// OffsetStream.h

#pragma once

#ifndef __OFFSETSTREAM_H
#define __OFFSETSTREAM_H

#include "Interface/IInOutStreams.h"

class COffsetOutStream: 
  public IOutStream,
  public CComObjectRoot
{
  UINT64 m_Offset;
  CComPtr<IOutStream> m_Stream;
public:
  HRESULT Init(IOutStream *aStream, UINT64 anOffset);
  
BEGIN_COM_MAP(COffsetOutStream)
  COM_INTERFACE_ENTRY(IOutStream)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(COffsetOutStream)
DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(Seek)(INT64 anOffset, UINT32 aSeekOrigin, UINT64 *aNewPosition);
  STDMETHOD(SetSize)(INT64 aNewSize);
};

#endif
