// Stream/OutByte.h

#pragma once

#ifndef __STREAM_OUTBYTE_H
#define __STREAM_OUTBYTE_H

#include "Interface/IInOutStreams.h"
#include "Common/Types.h"

namespace NStream {

class COutByteWriteException
{
public:
  HRESULT Result;
  COutByteWriteException(HRESULT result): Result (result) {}
};

class COutByte
{
  BYTE *_buffer;
  UINT32 _pos;
  UINT32 _bufferSize;
  CComPtr<ISequentialOutStream> _stream;
  UINT64 _processedSize;

  void WriteBlock();
public:
  COutByte(UINT32 bufferSize = (1 << 20));
  ~COutByte();

  void Init(ISequentialOutStream *stream);
  HRESULT Flush();
  void ReleaseStream();

  void WriteByte(BYTE b)
  {
    _buffer[_pos++] = b;
    if(_pos >= _bufferSize)
      WriteBlock();
  }
  void WriteBytes(const void *data, UINT32 size)
  {
    for (UINT32 i = 0; i < size; i++)
      WriteByte(((const BYTE *)data)[i]);
  }

  UINT64 GetProcessedSize() const { return _processedSize + _pos; }
};

}

#endif
