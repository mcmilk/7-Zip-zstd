// Stream/InByte.cpp

#include "stdafx.h"

#include "Stream/InByte.h"

namespace NStream{

CInByte::CInByte(UINT32 bufferSize):
  _bufferSize(bufferSize),
  _bufferBase(0)
{
  _bufferBase = new BYTE[_bufferSize];
}

CInByte::~CInByte()
{
  delete []_bufferBase;
}

void CInByte::Init(ISequentialInStream *stream)
{
  _stream = stream;
  _processedSize = 0;
  _buffer = _bufferBase;
  _bufferLimit = _buffer;
  _streamWasExhausted = false;
}

bool CInByte::ReadBlock()
{
  if (_streamWasExhausted)
    return false;
  _processedSize += (_buffer - _bufferBase);
  UINT32 numProcessedBytes;
  HRESULT result = _stream->ReadPart(_bufferBase, _bufferSize, &numProcessedBytes);
  if (result != S_OK)
    throw CInByteReadException(result);
  _buffer = _bufferBase;
  _bufferLimit = _buffer + numProcessedBytes;
  _streamWasExhausted = (numProcessedBytes == 0);
  return (!_streamWasExhausted);
}

}
