// 7zSpecStream.h

#pragma once

#ifndef __7Z_SPEC_STREAM_H
#define __7Z_SPEC_STREAM_H

#include "../../IStream.h"
#include "../../ICoder.h"
#include "../../../Common/MyCom.h"

class CSequentialInStreamSizeCount2: 
  public ISequentialInStream,
  public ICompressGetSubStreamSize,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialInStream> _stream;
  CMyComPtr<ICompressGetSubStreamSize> _getSubStreamSize;
  UINT64 _size;
public:
  void Init(ISequentialInStream *stream)
  {
    _stream = stream;
    _getSubStreamSize = 0;
    _stream.QueryInterface(IID_ICompressGetSubStreamSize, &_getSubStreamSize);
    _size = 0;
  }
  UINT64 GetSize() const { return _size; }

  MY_UNKNOWN_IMP1(ICompressGetSubStreamSize)

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);

  STDMETHOD(GetSubStreamSize)(UINT64 subStream, UINT64 *value);
};


#endif