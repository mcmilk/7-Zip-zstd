// Stream/WindowOut.h

#pragma once

#ifndef __STREAM_WINDOWOUT_H
#define __STREAM_WINDOWOUT_H

#include "Interface/IInOutStreams.h"

namespace NStream {
namespace NWindow {

// m_KeepSizeBefore: how mach BYTEs must be in buffer before m_Pos;
// m_KeepSizeAfter: how mach BYTEs must be in buffer after m_Pos;
// m_KeepSizeReserv: how mach BYTEs must be in buffer for Moving Reserv; 
//                    must be >= aKeepSizeAfter; // test it

class COutWriteException
{
public:
  HRESULT m_Result;
  COutWriteException(HRESULT aResult): m_Result (aResult) {}
};


class COut
{
  BYTE  *m_Buffer;
  UINT32	m_Pos;
  UINT32 m_PosLimit;
  UINT32 m_KeepSizeBefore;
  UINT32 m_KeepSizeAfter;
  UINT32 m_KeepSizeReserv;
  UINT32 m_StreamPos;

  UINT32 m_WindowSize;
  UINT32 m_MoveFrom;

  ISequentialOutStream *m_Stream;

  virtual void MoveBlockBackward();
public:
  COut(): m_Buffer(0), m_Stream(0) {}
  ~COut();
	void Create(UINT32 aKeepSizeBefore, 
      UINT32 aKeepSizeAfter, UINT32 aKeepSizeReserv = (1<<17));
  void SetWindowSize(UINT32 aWindowSize);

  void Init(ISequentialOutStream *aStream, bool aSolid = false);
  HRESULT Flush();
  void ReleaseStream();
  
  UINT32 GetCurPos() const { return m_Pos; }
  const BYTE *GetPointerToCurrentPos() const { return m_Buffer + m_Pos;};

  void CopyBackBlock(UINT32 aDistance, UINT32 aLen)
  {
    if (m_Pos >= m_PosLimit)
      MoveBlockBackward();  
    BYTE *p = m_Buffer + m_Pos;
    aDistance++;
    for(UINT32 i = 0; i < aLen; i++)
      p[i] = p[i - aDistance];
    m_Pos += aLen;
  }

  void PutOneByte(BYTE aByte)
  {
    if (m_Pos >= m_PosLimit)
      MoveBlockBackward();  
    m_Buffer[m_Pos++] = aByte;
  }

  BYTE GetOneByte(UINT32 anIndex) const
  {
    return m_Buffer[m_Pos + anIndex];
  }

  BYTE *GetBuffer() const { return m_Buffer; }
};

}}

#endif
