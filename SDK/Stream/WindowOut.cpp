// Stream/Window/Out.cpp

#include "StdAfx.h"

#include "Stream/WindowOut.h"

namespace NStream {
namespace NWindow {

void COut::Create(UINT32 windowSize)
{
  _pos = 0;
  _streamPos = 0;
  UINT32 newBlockSize = windowSize;
  const UINT32 kMinBlockSize = 1;
  if (newBlockSize < kMinBlockSize)
    newBlockSize = kMinBlockSize;
  if (_buffer != 0 && _windowSize == newBlockSize)
    return;
  delete []_buffer;
  _buffer = 0;
  _windowSize = newBlockSize;
  _buffer = new BYTE[_windowSize];
}

COut::~COut()
{
  ReleaseStream();
  delete []_buffer;
}

/*
void COut::SetWindowSize(UINT32 windowSize)
{
  _windowSize = windowSize;
}
*/

void COut::Init(ISequentialOutStream *stream, bool solid)
{
  ReleaseStream();
  _stream = stream;
  _stream->AddRef();

  if(!solid)
  {
    _streamPos = 0;
    _pos = 0;
  }
}

void COut::ReleaseStream()
{
  if(_stream != 0)
  {
    // Flush(); // Test it
    _stream->Release();
    _stream = 0;
  }
}

void COut::FlushWithCheck()
{
  HRESULT result = Flush();
  if (result != S_OK)
    throw COutWriteException(result);
}

HRESULT COut::Flush()
{
  UINT32 size = _pos - _streamPos;
  if(size == 0)
    return S_OK;
  UINT32 processedSize;
  HRESULT result = _stream->Write(_buffer + _streamPos, size, &processedSize);
  if (result != S_OK)
    return result;
  if (size != processedSize)
    return E_FAIL;
  if (_pos >= _windowSize)
    _pos = 0;
  _streamPos = _pos;
  return S_OK;
}

}}
