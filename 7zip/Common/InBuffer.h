// InBuffer.h

#ifndef __INBUFFER_H
#define __INBUFFER_H

#include "../IStream.h"
#include "../../Common/MyCom.h"

#ifndef _NO_EXCEPTIONS
class CInBufferException
{
public:
  HRESULT ErrorCode;
  CInBufferException(HRESULT errorCode): ErrorCode(errorCode) {}
};
#endif

class CInBuffer
{
  UInt64 _processedSize;
  Byte *_bufferBase;
  UInt32 _bufferSize;
  Byte *_buffer;
  Byte *_bufferLimit;
  CMyComPtr<ISequentialInStream> _stream;
  bool _wasFinished;

  bool ReadBlock();

public:
  #ifdef _NO_EXCEPTIONS
  HRESULT ErrorCode;
  #endif

  CInBuffer();
  ~CInBuffer() { Free(); }

  bool Create(UInt32 bufferSize);
  void Free();
  
  void SetStream(ISequentialInStream *stream);
  void Init();
  void ReleaseStream() { _stream.Release(); }

  bool ReadByte(Byte &b)
  {
    if(_buffer >= _bufferLimit)
      if(!ReadBlock())
        return false;
    b = *_buffer++;
    return true;
  }
  Byte ReadByte()
  {
    if(_buffer >= _bufferLimit)
      if(!ReadBlock())
        return 0xFF;
    return *_buffer++;
  }
  void ReadBytes(void *data, UInt32 size, UInt32 &processedSize)
  {
    for(processedSize = 0; processedSize < size; processedSize++)
      if (!ReadByte(((Byte *)data)[processedSize]))
        return;
  }
  bool ReadBytes(void *data, UInt32 size)
  {
    UInt32 processedSize;
    ReadBytes(data, size, processedSize);
    return (processedSize == size);
  }
  UInt64 GetProcessedSize() const { return _processedSize + (_buffer - _bufferBase); }
  bool WasFinished() const { return _wasFinished; }
};

#endif
