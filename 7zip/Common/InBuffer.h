// InBuffer.h

// #pragma once

#ifndef __INBUFFER_H
#define __INBUFFER_H

#include "../IStream.h"

class CInBufferException
{
public:
  HRESULT ErrorCode;
  CInBufferException(HRESULT errorCode): ErrorCode(errorCode) {}
};

class CInBuffer
{
  UINT64 _processedSize;
  BYTE *_bufferBase;
  UINT32 _bufferSize;
  BYTE *_buffer;
  BYTE *_bufferLimit;
  ISequentialInStream *_stream;
  bool _streamWasExhausted;

  bool ReadBlock();

public:
  CInBuffer(UINT32 bufferSize = 0x100000);
  ~CInBuffer();
  
  void Init(ISequentialInStream *stream);
  /*
  void ReleaseStream()
    { _stream.Release(); }
  */

  bool ReadByte(BYTE &b)
    {
      if(_buffer >= _bufferLimit)
        if(!ReadBlock())
          return false;
      b = *_buffer++;
      return true;
    }
  BYTE ReadByte()
    {
      if(_buffer >= _bufferLimit)
        if(!ReadBlock())
          return 0x0;
      return *_buffer++;
    }
  void ReadBytes(void *data, UINT32 size, UINT32 &processedSize)
    {
      for(processedSize = 0; processedSize < size; processedSize++)
        if (!ReadByte(((BYTE *)data)[processedSize]))
          return;
    }
  bool ReadBytes(void *data, UINT32 size)
    {
      UINT32 processedSize;
      ReadBytes(data, size, processedSize);
      return (processedSize == size);
    }
  UINT64 GetProcessedSize() const { return _processedSize + (_buffer - _bufferBase); }
};

#endif
