// BitlEncoder.h -- the Least Significant Bit of byte is First

#ifndef __BITL_ENCODER_H
#define __BITL_ENCODER_H

#include "../Common/OutBuffer.h"

class CBitlEncoder
{
  COutBuffer m_Stream;
  unsigned m_BitPos;
  Byte m_CurByte;
public:
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialOutStream *outStream) { m_Stream.SetStream(outStream); }
  void ReleaseStream() { m_Stream.ReleaseStream(); }
  UInt32 GetBitPosition() const { return (8 - m_BitPos); }
  UInt64 GetProcessedSize() const { return m_Stream.GetProcessedSize() + (8 - m_BitPos + 7) /8; }
  void Init()
  {
    m_Stream.Init();
    m_BitPos = 8;
    m_CurByte = 0;
  }
  HRESULT Flush()
  {
    FlushByte();
    return m_Stream.Flush();
  }
  void FlushByte()
  {
    if (m_BitPos < 8)
      m_Stream.WriteByte(m_CurByte);
    m_BitPos = 8;
    m_CurByte = 0;
  }
  void WriteBits(UInt32 value, unsigned numBits)
  {
    while (numBits > 0)
    {
      if (numBits < m_BitPos)
      {
        m_CurByte |= (value & ((1 << numBits) - 1)) << (8 - m_BitPos);
        m_BitPos -= numBits;
        return;
      }
      numBits -= m_BitPos;
      m_Stream.WriteByte((Byte)(m_CurByte | (value << (8 - m_BitPos))));
      value >>= m_BitPos;
      m_BitPos = 8;
      m_CurByte = 0;
    }
  }
  void WriteByte(Byte b) { m_Stream.WriteByte(b);}
};

#endif
