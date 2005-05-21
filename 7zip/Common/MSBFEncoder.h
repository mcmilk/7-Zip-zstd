// Stream/MSBFEncoder.h

#ifndef __STREAM_MSBFENCODER_H
#define __STREAM_MSBFENCODER_H

#include "Common/Defs.h"
#include "../IStream.h"
#include "OutBuffer.h"

namespace NStream {
namespace NMSBF {

template<class TOutByte>
class CEncoder
{
  TOutByte m_Stream;
  int m_BitPos;
  Byte m_CurByte;
public:
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialOutStream *outStream) { m_Stream.SetStream(outStream);}
  void ReleaseStream() { m_Stream.ReleaseStream(); }
  void Init()
  {
    m_Stream.Init();
    m_BitPos = 8; 
    m_CurByte = 0;
  }
  HRESULT Flush()
  {
    if(m_BitPos < 8)
      WriteBits(0, m_BitPos);
    return m_Stream.Flush();
  }

  void WriteBits(UInt32 value, int numBits)
  {
    while(numBits > 0)
    {
      int numNewBits = MyMin(numBits, m_BitPos);
      numBits -= numNewBits;
      
      m_CurByte <<= numNewBits;
      UInt32 newBits = value >> numBits;
      m_CurByte |= Byte(newBits);
      value -= (newBits << numBits);
      
      m_BitPos -= numNewBits;
      
      if (m_BitPos == 0)
      {
        m_Stream.WriteByte(m_CurByte);
        m_BitPos = 8;
      }
    }
  }
  UInt64 GetProcessedSize() const { 
      return m_Stream.GetProcessedSize() + (8 - m_BitPos + 7) / 8; }
};

}}

#endif
