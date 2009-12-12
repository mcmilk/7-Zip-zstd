// BitmEncoder.h -- the Most Significant Bit of byte is First

#ifndef __BITM_ENCODER_H
#define __BITM_ENCODER_H

#include "../IStream.h"

template<class TOutByte>
class CBitmEncoder
{
  TOutByte m_Stream;
  unsigned m_BitPos;
  Byte m_CurByte;
public:
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialOutStream *outStream) { m_Stream.SetStream(outStream);}
  void ReleaseStream() { m_Stream.ReleaseStream(); }
  UInt64 GetProcessedSize() const { return m_Stream.GetProcessedSize() + (8 - m_BitPos + 7) / 8; }
  void Init()
  {
    m_Stream.Init();
    m_BitPos = 8;
    m_CurByte = 0;
  }
  HRESULT Flush()
  {
    if (m_BitPos < 8)
      WriteBits(0, m_BitPos);
    return m_Stream.Flush();
  }
  void WriteBits(UInt32 value, unsigned numBits)
  {
    while (numBits > 0)
    {
      if (numBits < m_BitPos)
      {
        m_CurByte |= ((Byte)value << (m_BitPos -= numBits));
        return;
      }
      numBits -= m_BitPos;
      UInt32 newBits = (value >> numBits);
      value -= (newBits << numBits);
      m_Stream.WriteByte((Byte)(m_CurByte | newBits));
      m_BitPos = 8;
      m_CurByte = 0;
    }
  }
};

#endif
