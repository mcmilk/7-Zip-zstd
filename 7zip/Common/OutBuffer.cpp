// OutByte.cpp

#include "StdAfx.h"

#include "OutBuffer.h"

#include "../../Common/Alloc.h"

bool COutBuffer::Create(UInt32 bufferSize)
{
  const UInt32 kMinBlockSize = 1;
  if (bufferSize < kMinBlockSize)
    bufferSize = kMinBlockSize;
  if (_buffer != 0 && _bufferSize == bufferSize)
    return true;
  Free();
  _bufferSize = bufferSize;
  _buffer = (Byte *)::BigAlloc(bufferSize);
  return (_buffer != 0);
}

void COutBuffer::Free()
{
  BigFree(_buffer);
  _buffer = 0;
}

void COutBuffer::SetStream(ISequentialOutStream *stream)
{
  _stream = stream;
}

void COutBuffer::Init()
{
  _processedSize = 0;
  _pos = 0;
  #ifdef _NO_EXCEPTIONS
  ErrorCode = S_OK;
  #endif
}

HRESULT COutBuffer::Flush()
{
  if (_pos == 0)
    return S_OK;
  UInt32 processedSize;
  HRESULT result = _stream->Write(_buffer, _pos, &processedSize);
  if (result != S_OK)
    return result;
  if (_pos != processedSize)
    return E_FAIL;
  _processedSize += processedSize;
  _pos = 0;
  return S_OK;
}

void COutBuffer::WriteBlock()
{
  #ifdef _NO_EXCEPTIONS
  if (ErrorCode != S_OK)
    return;
  #endif
  HRESULT result = Flush();
  #ifdef _NO_EXCEPTIONS
  ErrorCode = result;
  #else
  if (result != S_OK)
    throw COutBufferException(result);
  #endif
}
