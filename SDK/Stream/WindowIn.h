// Stream/WindowIn.h

#pragma once

#ifndef __STREAM_WINDOWIN_H
#define __STREAM_WINDOWIN_H

#include "Interface/IInOutStreams.h"

namespace NStream {
namespace NWindow {

class CIn
{
  BYTE  *m_BufferBase; // pointer to buffer with data
  CComPtr<ISequentialInStream> m_Stream;
  UINT32 m_PosLimit;  // offset (from m_Buffer) of first byte when new block reading must be done
  bool m_StreamEndWasReached; // if (true) then m_StreamPos shows real end of stream

  const BYTE *m_PointerToLastSafePosition;

protected:
  BYTE  *m_Buffer;   // Pointer to virtual Buffer begin
  UINT32 m_BlockSize;  // Size of Allocated memory block
  UINT32 m_Pos;             // offset (from m_Buffer) of curent byte
  UINT32 m_KeepSizeBefore;  // how many BYTEs must be kept in buffer before m_Pos
  UINT32 m_KeepSizeAfter;   // how many BYTEs must be kept buffer after m_Pos
  UINT32 m_KeepSizeReserv;  // how many BYTEs must be kept as reserv
  UINT32 m_StreamPos;   // offset (from m_Buffer) of first not read byte from Stream

  virtual void MoveBlock(UINT32 anOffset);
  virtual HRESULT ReadBlock();
public:
  CIn();
  void Create(UINT32 aKeepSizeBefore, UINT32 aKeepSizeAfter, 
      UINT32 aKeepSizeReserv = (1<<17));
  ~CIn();

  HRESULT Init(ISequentialInStream *aStream);
  void ReleaseStream();

  BYTE *GetBuffer() const { return m_Buffer; }

  const BYTE *GetPointerToCurrentPos() const { return m_Buffer + m_Pos; }

  HRESULT MovePos()
  {
    m_Pos++;
    if (m_Pos > m_PosLimit)
    {
      const BYTE *aPointerToPostion = m_Buffer + m_Pos;
      if(aPointerToPostion > m_PointerToLastSafePosition)
        MoveBlock((m_Buffer + m_Pos - m_KeepSizeBefore) - m_BufferBase);
      return ReadBlock();
    }
    else
      return S_OK;
  }
  // BYTE GetCurrentByte()const;
  BYTE GetIndexByte(UINT32 anIndex)const
    {  return m_Buffer[m_Pos + anIndex]; }

  // UINT32 GetCurPos()const { return m_Pos;};
  // BYTE *GetBufferBeg()const { return m_Buffer;};

  // aIndex + aLimit have not to exceed m_KeepSizeAfter;
  UINT32 GetMatchLen(UINT32 aIndex, UINT32 aBack, UINT32 aLimit) const
  {  
    if(m_StreamEndWasReached)
      if ((m_Pos + aIndex) + aLimit > m_StreamPos)
        aLimit = m_StreamPos - (m_Pos + aIndex);
      aBack++;
      BYTE *pby = m_Buffer + m_Pos + aIndex;
      for(UINT32 i = 0; i < aLimit && pby[i] == pby[i - aBack]; i++);
      return i;
  }

  UINT32 GetNumAvailableBytes() const { return m_StreamPos - m_Pos; }

  void ReduceOffsets(UINT32 aSubValue)
  {
    m_Buffer += aSubValue;
    m_PosLimit -= aSubValue;
    m_Pos -= aSubValue;
    m_StreamPos -= aSubValue;
  }

};

}}

#endif
