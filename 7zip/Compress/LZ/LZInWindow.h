// LZInWindow.h

// #pragma once

#ifndef __LZ_IN_WINDOW_H
#define __LZ_IN_WINDOW_H

#include "../../IStream.h"

class CLZInWindow
{
  BYTE  *_bufferBase; // pointer to buffer with data
  ISequentialInStream *_stream;
  UINT32 _posLimit;  // offset (from _buffer) of first byte when new block reading must be done
  bool _streamEndWasReached; // if (true) then _streamPos shows real end of stream
  const BYTE *_pointerToLastSafePosition;
protected:
  BYTE  *_buffer;   // Pointer to virtual Buffer begin
  UINT32 _blockSize;  // Size of Allocated memory block
  UINT32 _pos;             // offset (from _buffer) of curent byte
  UINT32 _keepSizeBefore;  // how many BYTEs must be kept in buffer before _pos
  UINT32 _keepSizeAfter;   // how many BYTEs must be kept buffer after _pos
  UINT32 _keepSizeReserv;  // how many BYTEs must be kept as reserv
  UINT32 _streamPos;   // offset (from _buffer) of first not read byte from Stream

  virtual void BeforeMoveBlock() {};
  virtual void AfterMoveBlock() {};
  void MoveBlock();
  virtual HRESULT ReadBlock();
  void Free();
public:
  CLZInWindow(): _bufferBase(0) {}
  ~CLZInWindow();
  void Create(UINT32 keepSizeBefore, UINT32 keepSizeAfter, 
      UINT32 keepSizeReserv = (1<<17));

  HRESULT Init(ISequentialInStream *stream);
  // void ReleaseStream();

  BYTE *GetBuffer() const { return _buffer; }

  const BYTE *GetPointerToCurrentPos() const { return _buffer + _pos; }

  HRESULT MovePos()
  {
    _pos++;
    if (_pos > _posLimit)
    {
      const BYTE *pointerToPostion = _buffer + _pos;
      if(pointerToPostion > _pointerToLastSafePosition)
        MoveBlock();
      return ReadBlock();
    }
    else
      return S_OK;
  }
  // BYTE GetCurrentByte()const;
  BYTE GetIndexByte(UINT32 index)const
    {  return _buffer[_pos + index]; }

  // UINT32 GetCurPos()const { return _pos;};
  // BYTE *GetBufferBeg()const { return _buffer;};

  // index + limit have not to exceed _keepSizeAfter;
  UINT32 GetMatchLen(UINT32 index, UINT32 back, UINT32 limit) const
  {  
    if(_streamEndWasReached)
      if ((_pos + index) + limit > _streamPos)
        limit = _streamPos - (_pos + index);
    back++;
    BYTE *pby = _buffer + _pos + index;
    UINT32 i;
    for(i = 0; i < limit && pby[i] == pby[i - back]; i++);
    return i;
  }

  UINT32 GetNumAvailableBytes() const { return _streamPos - _pos; }

  void ReduceOffsets(UINT32 subValue)
  {
    _buffer += subValue;
    _posLimit -= subValue;
    _pos -= subValue;
    _streamPos -= subValue;
  }

};

#endif
