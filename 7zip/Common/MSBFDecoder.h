// MSBFDecoder.h
// the Most Significant Bit of byte is First

#ifndef __STREAM_MSBFDECODER_H
#define __STREAM_MSBFDECODER_H

namespace NStream {
namespace NMSBF {

const int kNumBigValueBits = 8 * 4;
const int kNumValueBytes = 3;
const int kNumValueBits = 8  * kNumValueBytes;

const UInt32 kMask = (1 << kNumValueBits) - 1;

template<class TInByte>
class CDecoder
{
  TInByte m_Stream;
  UInt32 m_BitPos;
  UInt32 m_Value;
public:
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialInStream *inStream) { m_Stream.SetStream(inStream);}
  void ReleaseStream() { m_Stream.ReleaseStream();}

  void Init()
  {
    m_Stream.Init();
    m_BitPos = kNumBigValueBits; 
    Normalize();
  }
  
  UInt64 GetProcessedSize() const 
    { return m_Stream.GetProcessedSize() - (kNumBigValueBits - m_BitPos) / 8; }
  
  void Normalize()
  {
    for (;m_BitPos >= 8; m_BitPos -= 8)
      m_Value = (m_Value << 8) | m_Stream.ReadByte();
  }

  UInt32 GetValue(UInt32 numBits) const
  {
    // return (m_Value << m_BitPos) >> (kNumBigValueBits - numBits);
    return ((m_Value >> (8 - m_BitPos)) & kMask) >> (kNumValueBits - numBits);
  }
  
  void MovePos(UInt32 numBits)
  {
    m_BitPos += numBits;
    Normalize();
  }
  
  UInt32 ReadBits(UInt32 numBits)
  {
    UInt32 res = GetValue(numBits);
    MovePos(numBits);
    return res;
  }
};

}}

#endif
