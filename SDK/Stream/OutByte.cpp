// Stream/OutByte.cpp

#include "StdAfx.h"

#include "Stream/OutByte.h"

namespace NStream {

COutByte::COutByte(UINT32 bufferSize):
  _bufferSize(bufferSize)
{
  _buffer = new BYTE[_bufferSize];
}

COutByte::~COutByte()
{
  delete []_buffer;
}

void COutByte::Init(ISequentialOutStream *stream)
{
  _stream = stream;
  _processedSize = 0;
  _pos = 0;
}

void COutByte::ReleaseStream()
{
  _stream.Release();
}


HRESULT COutByte::Flush()
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

void COutByte::WriteBlock()
{
  HRESULT result = Flush();
  if (result != S_OK)
    throw COutByteWriteException(result);
}

}