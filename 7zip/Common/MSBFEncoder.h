// Stream/MSBFEncoder.h
// the Most Significant Bit of byte is First

#pragma once

#ifndef __STREAM_MSBFENCODER_H
#define __STREAM_MSBFENCODER_H

#include "Common/Defs.h"
#include "Interface/IInOutStreams.h"

namespace NStream {
namespace NMSBF {

template<class TOutByte>
class CEncoder
{
  TOutByte m_Stream;
  UINT32 m_BitPos;
  BYTE m_CurByte;
public:
  void Init(ISequentialOutStream *aStream)
  {
    m_Stream.Init(aStream);
    m_BitPos = 8; 
    m_CurByte = 0;
  }
  HRESULT Flush()
  {
    if(m_BitPos < 8)
      WriteBits(0, m_BitPos);
    return m_Stream.Flush();
  }

  void ReleaseStream()
  {
    m_Stream.ReleaseStream();
  }

  void WriteBits(UINT32 aValue, UINT32 aNumBits)
  {
    while(aNumBits > 0)
    {
      UINT32 aNumNewBits = MyMin(aNumBits, m_BitPos);
      aNumBits -= aNumNewBits;
      
      
      m_CurByte <<= aNumNewBits;
      UINT32 aNewBits = aValue >> aNumBits;
      m_CurByte |= BYTE(aNewBits);
      aValue -= (aNewBits << aNumBits);
      
      
      m_BitPos -= aNumNewBits;
      
      if (m_BitPos == 0)
      {
        m_Stream.WriteByte(m_CurByte);
        m_BitPos = 8;
      }
    }
  }
  UINT64 GetProcessedSize() const { 
      return m_Stream.GetProcessedSize() + (8 - m_BitPos + 7) /8; }
};

}}

#endif

