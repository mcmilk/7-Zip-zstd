// OffsetStream.h

#pragma once

#ifndef __OFFSETSTREAM_H
#define __OFFSETSTREAM_H

#include "Common/MyCom.h"
#include "../IStream.h"

class COffsetOutStream: 
  public IOutStream,
  public CMyUnknownImp
{
  UINT64 _offset;
  CMyComPtr<IOutStream> _stream;
public:
  HRESULT Init(IOutStream *stream, UINT64 offset);
  
  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition);
  STDMETHOD(SetSize)(INT64 newSize);
};

#endif
