// Stream/MSBFEncoder.h
// the Most Significant Bit of byte is First

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
  UInt32 m_BitPos;
  Byte m_CurByte;
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

  void WriteBits(UInt32 aValue, UInt32 aNumBits)
  {
    while(aNumBits > 0)
    {
      UInt32 aNumNewBits = MyMin(aNumBits, m_BitPos);
      aNumBits -= aNumNewBits;
      
      
      m_CurByte <<= aNumNewBits;
      UInt32 aNewBits = aValue >> aNumBits;
      m_CurByte |= Byte(aNewBits);
      aValue -= (aNewBits << aNumBits);
      
      
      m_BitPos -= aNumNewBits;
      
      if (m_BitPos == 0)
      {
        m_Stream.WriteByte(m_CurByte);
        m_BitPos = 8;
      }
    }
  }
  UInt64 GetProcessedSize() const { 
      return m_Stream.GetProcessedSize() + (8 - m_BitPos + 7) /8; }
};

}}

#endif
