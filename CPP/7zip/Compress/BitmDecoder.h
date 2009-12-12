// BitmDecoder.h -- the Most Significant Bit of byte is First

#ifndef __BITM_DECODER_H
#define __BITM_DECODER_H

#include "../IStream.h"

namespace NBitm {

const unsigned kNumBigValueBits = 8 * 4;
const unsigned kNumValueBytes = 3;
const unsigned kNumValueBits = 8  * kNumValueBytes;

const UInt32 kMask = (1 << kNumValueBits) - 1;

template<class TInByte>
class CDecoder
{
  unsigned m_BitPos;
  UInt32 m_Value;
public:
  TInByte m_Stream;
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialInStream *inStream) { m_Stream.SetStream(inStream);}
  void ReleaseStream() { m_Stream.ReleaseStream();}

  void Init()
  {
    m_Stream.Init();
    m_BitPos = kNumBigValueBits;
    Normalize();
  }
  
  UInt64 GetProcessedSize() const { return m_Stream.GetProcessedSize() - (kNumBigValueBits - m_BitPos) / 8; }
  
  void Normalize()
  {
    for (;m_BitPos >= 8; m_BitPos -= 8)
      m_Value = (m_Value << 8) | m_Stream.ReadByte();
  }

  UInt32 GetValue(unsigned numBits) const
  {
    // return (m_Value << m_BitPos) >> (kNumBigValueBits - numBits);
    return ((m_Value >> (8 - m_BitPos)) & kMask) >> (kNumValueBits - numBits);
  }
  
  void MovePos(unsigned numBits)
  {
    m_BitPos += numBits;
    Normalize();
  }
  
  UInt32 ReadBits(unsigned numBits)
  {
    UInt32 res = GetValue(numBits);
    MovePos(numBits);
    return res;
  }

  void AlignToByte() { MovePos((32 - m_BitPos) & 7); }
};

}

#endif
