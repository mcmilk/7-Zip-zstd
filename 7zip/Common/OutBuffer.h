// OutBuffer.h

// #pragma once

#ifndef __OUTBUFFER_H
#define __OUTBUFFER_H

#include "../IStream.h"

class COutBufferException
{
public:
  HRESULT ErrorCode;
  COutBufferException(HRESULT errorCode): ErrorCode(errorCode) {}
};

class COutBuffer
{
  BYTE *_buffer;
  UINT32 _pos;
  UINT32 _bufferSize;
  ISequentialOutStream *_stream;
  UINT64 _processedSize;

  void WriteBlock();
public:
  COutBuffer(UINT32 bufferSize = (1 << 20));
  ~COutBuffer();

  void Init(ISequentialOutStream *stream);
  HRESULT Flush();
  // void ReleaseStream();

  void *GetBuffer(UINT32 &sizeAvail)
  {
    sizeAvail = _bufferSize - _pos;
    return _buffer + _pos;
  }
  void MovePos(UINT32 num)
  {
    _pos += num;
    if(_pos >= _bufferSize)
      WriteBlock();
  }


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

#endif
