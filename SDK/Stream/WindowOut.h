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
  UINT32 m_Pos;
  UINT32 m_WindowSize;
  UINT32 m_StreamPos;
  ISequentialOutStream *m_Stream;

public:
  COut(): m_Buffer(0), m_Stream(0) {}
  ~COut();
	void Create(UINT32 aWindowSize);

  void Init(ISequentialOutStream *aStream, bool aSolid = false);
  HRESULT Flush();
  void ReleaseStream();
  
  // UINT32 GetCurPos() const { return m_Pos; }
  // const BYTE *GetPointerToCurrentPos() const { return m_Buffer + m_Pos;};

  void CopyBackBlock(UINT32 aDistance, UINT32 aLen)
  {
		UINT32 aPos = m_Pos - aDistance - 1;
  	if (aPos >= m_WindowSize)
  		aPos += m_WindowSize;
		for(; aLen > 0; aLen--)
		{
			if (aPos >= m_WindowSize)
				aPos = 0;
			m_Buffer[m_Pos++] = m_Buffer[aPos++];
			if (m_Pos >= m_WindowSize)
				Flush();  
			// PutOneByte(GetOneByte(0 - aDistance));
		}
  }

  void PutOneByte(BYTE aByte)
  {
		m_Buffer[m_Pos++] = aByte;
		if (m_Pos >= m_WindowSize)
			Flush();  
  }

  BYTE GetOneByte(UINT32 anIndex) const
  {
		UINT32 aPos = m_Pos + anIndex;
		if (aPos >= m_WindowSize)
			aPos += m_WindowSize;
		return m_Buffer[aPos]; 
  }

  // BYTE *GetBuffer() const { return m_Buffer; }
};

}}

#endif
