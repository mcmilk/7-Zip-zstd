// OffsetStream.h

#pragma once

#ifndef __OFFSETSTREAM_H
#define __OFFSETSTREAM_H

#include "Interface/IInOutStreams.h"

class COffsetOutStream: 
  public IOutStream,
  public CComObjectRoot
{
  UINT64 _offset;
  CComPtr<IOutStream> _stream;
public:
  HRESULT Init(IOutStream *stream, UINT64 offset);
  
BEGIN_COM_MAP(COffsetOutStream)
  COM_INTERFACE_ENTRY(IOutStream)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(COffsetOutStream)
DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition);
  STDMETHOD(SetSize)(INT64 newSize);
};

#endif
