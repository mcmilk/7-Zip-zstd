// InBuffer.cpp

#include "stdafx.h"

#include "InBuffer.h"

CInBuffer::CInBuffer(UINT32 bufferSize):
  _bufferSize(bufferSize),
  _bufferBase(0)
{
  _bufferBase = new BYTE[_bufferSize];
}

CInBuffer::~CInBuffer()
{
  delete []_bufferBase;
}

void CInBuffer::Init(ISequentialInStream *stream)
{
  _stream = stream;
  _processedSize = 0;
  _buffer = _bufferBase;
  _bufferLimit = _buffer;
  _streamWasExhausted = false;
}

bool CInBuffer::ReadBlock()
{
  if (_streamWasExhausted)
    return false;
  _processedSize += (_buffer - _bufferBase);
  UINT32 numProcessedBytes;
  HRESULT result = _stream->ReadPart(_bufferBase, _bufferSize, &numProcessedBytes);
  if (result != S_OK)
    throw CInBufferException(result);
  _buffer = _bufferBase;
  _bufferLimit = _buffer + numProcessedBytes;
  _streamWasExhausted = (numProcessedBytes == 0);
  return (!_streamWasExhausted);
}
