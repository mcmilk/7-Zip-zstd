// InBuffer.cpp

#include "StdAfx.h"

#include "InBuffer.h"

#include "../../Common/Alloc.h"

CInBuffer::CInBuffer(): 
  _bufferBase(0), 
  _bufferSize(0),
  _buffer(0), 
  _bufferLimit(0), 
  _stream(0) 
{}

bool CInBuffer::Create(UInt32 bufferSize)
{
  const UInt32 kMinBlockSize = 1;
  if (bufferSize < kMinBlockSize)
    bufferSize = kMinBlockSize;
  if (_bufferBase != 0 && _bufferSize == bufferSize)
    return true;
  Free();
  _bufferSize = bufferSize;
  _bufferBase = (Byte *)::BigAlloc(bufferSize);
  return (_bufferBase != 0);
}

void CInBuffer::Free()
{
  BigFree(_bufferBase);
  _bufferBase = 0;
}

void CInBuffer::SetStream(ISequentialInStream *stream)
{
  _stream = stream;
}

void CInBuffer::Init()
{
  _processedSize = 0;
  _buffer = _bufferBase;
  _bufferLimit = _buffer;
  _wasFinished = false;
  #ifdef _NO_EXCEPTIONS
  ErrorCode = S_OK;
  #endif
}

bool CInBuffer::ReadBlock()
{
  #ifdef _NO_EXCEPTIONS
  if (ErrorCode != S_OK)
    return false;
  #endif
  if (_wasFinished)
    return false;
  _processedSize += (_buffer - _bufferBase);
  UInt32 numProcessedBytes;
  HRESULT result = _stream->ReadPart(_bufferBase, _bufferSize, &numProcessedBytes);
  #ifdef _NO_EXCEPTIONS
  ErrorCode = result;
  #else
  if (result != S_OK)
    throw CInBufferException(result);
  #endif
  _buffer = _bufferBase;
  _bufferLimit = _buffer + numProcessedBytes;
  _wasFinished = (numProcessedBytes == 0);
  return (!_wasFinished);
}
