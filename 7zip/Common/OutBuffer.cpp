// Stream/OutByte.cpp

#include "StdAfx.h"

#include "OutBuffer.h"

COutBuffer::COutBuffer(UINT32 bufferSize):
  _bufferSize(bufferSize)
{
  _buffer = new BYTE[_bufferSize];
}

COutBuffer::~COutBuffer()
{
  delete []_buffer;
}

void COutBuffer::Init(ISequentialOutStream *stream)
{
  _stream = stream;
  _processedSize = 0;
  _pos = 0;
}

/*
void COutBuffer::ReleaseStream()
{
  _stream.Release();
}
*/


HRESULT COutBuffer::Flush()
{
  if (_pos == 0)
    return S_OK;
  UINT32 processedSize;
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
  HRESULT result = Flush();
  if (result != S_OK)
    throw COutBufferException(result);
}
