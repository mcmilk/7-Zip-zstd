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
  CComPtr<ISequentialInStream> _stream;
  CComPtr<ICompressGetSubStreamSize> _getSubStreamSize;
  UINT64 _size;
public:
  void Init(ISequentialInStream *stream)
  {
    _stream = stream;
    _getSubStreamSize = 0;
    _stream.QueryInterface(&_getSubStreamSize);
    _size = 0;
  }
  UINT64 GetSize() const { return _size; }
BEGIN_COM_MAP(CSequentialInStreamSizeCount2)
  COM_INTERFACE_ENTRY(ISequentialInStream)
  COM_INTERFACE_ENTRY(ICompressGetSubStreamSize)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialInStreamSizeCount2)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);

  STDMETHOD(GetSubStreamSize)(UINT64 subStream, UINT64 *value);
};


#endif