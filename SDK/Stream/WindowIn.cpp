// Stream/WindowIn.cpp

#include "StdAfx.h"

#include "WindowIn.h"

#include "Common/Defs.h"

#include "Windows/Defs.h"

namespace NStream {
namespace NWindow {

CIn::CIn():
  _bufferBase(0)
{}

void CIn::Free()
{
  delete []_bufferBase;
  _bufferBase = 0;
}

void CIn::Create(UINT32 keepSizeBefore, UINT32 keepSizeAfter, UINT32 keepSizeReserv)
{
  _keepSizeBefore = keepSizeBefore;
  _keepSizeAfter = keepSizeAfter;
  _keepSizeReserv = keepSizeReserv;
  _blockSize = keepSizeBefore + keepSizeAfter + keepSizeReserv;
  Free();
  _bufferBase = new BYTE[_blockSize];
  _pointerToLastSafePosition = _bufferBase + _blockSize - keepSizeAfter;
}

CIn::~CIn()
{
  Free();
}

HRESULT CIn::Init(ISequentialInStream *stream)
{
  _stream = stream;
  _buffer = _bufferBase;
  _pos = 0;
  _streamPos = 0;
  _streamEndWasReached = false;
  return ReadBlock();
}

void CIn::ReleaseStream()
{
  _stream.Release();
}


///////////////////////////////////////////
// ReadBlock

// In State:
//   (_buffer + _streamPos) <= (_bufferBase + _blockSize)
// Out State:
//   _posLimit <= _blockSize - _keepSizeAfter;
//   if(_streamEndWasReached == false):
//     _streamPos >= _pos + _keepSizeAfter
//     _posLimit = _streamPos - _keepSizeAfter;
//   else
//          
  
HRESULT CIn::ReadBlock()
{
  if(_streamEndWasReached)
    return S_OK;
  while(true)
  {
    UINT32 size = (_bufferBase + _blockSize) - (_buffer + _streamPos);
    if(size == 0)
      return S_OK;
    UINT32 numReadBytes;
    RETURN_IF_NOT_S_OK(_stream->ReadPart(_buffer + _streamPos, 
        size, &numReadBytes));
    if(numReadBytes == 0)
    {
      _posLimit = _streamPos;
      const BYTE *pointerToPostion = _buffer + _posLimit;
      if(pointerToPostion > _pointerToLastSafePosition)
        _posLimit = _pointerToLastSafePosition - _buffer;
      _streamEndWasReached = true;
      return S_OK;
    }
    _streamPos += numReadBytes;
    if(_streamPos >= _pos + _keepSizeAfter)
    {
      _posLimit = _streamPos - _keepSizeAfter;
      return S_OK;
    }
  }
}

void CIn::MoveBlock()
{
  BeforeMoveBlock();
  UINT32 offset = (_buffer + _pos - _keepSizeBefore) - _bufferBase;
  UINT32 numBytes = (_buffer + _streamPos) -  (_bufferBase + offset);
  memmove(_bufferBase, _bufferBase + offset, numBytes);
  _buffer -= offset;
  AfterMoveBlock();
}


}}
