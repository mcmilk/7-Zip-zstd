// Stream/MSBFDecoder.h
// the Most Significant Bit of byte is First

#pragma once

#ifndef __STREAM_MSBFDECODER_H
#define __STREAM_MSBFDECODER_H

namespace NStream {
namespace NMSBF {

const int kNumBigValueBits = 8 * 4;

const int kNumValueBytes = 3;
const int kNumValueBits = 8  * kNumValueBytes;

const UINT32 kMask = (1 << kNumValueBits) - 1;

template<class TInByte>
class CDecoder
{
  TInByte m_Stream;
  UINT32 m_BitPos;
  UINT32 m_Value;

public:
  
  void Init(ISequentialInStream *aStream)
  {
    m_Stream.Init(aStream);
    m_BitPos = kNumBigValueBits; 
    Normalize();
  }
  
  /*
  void ReleaseStream()
  {
    m_Stream.ReleaseStream();
  }
  */

  UINT64 GetProcessedSize() const 
    { return m_Stream.GetProcessedSize() - (kNumBigValueBits - m_BitPos) / 8; }
  
  void Normalize()
  {
    for (;m_BitPos >= 8; m_BitPos -= 8)
      m_Value = (m_Value << 8) | m_Stream.ReadByte();
  }

  UINT32 GetValue(UINT32 aNumBits) const
  {
    // return (m_Value << m_BitPos) >> (kNumBigValueBits - aNumBits);
    return ((m_Value >> (8 - m_BitPos)) & kMask) >> (kNumValueBits - aNumBits);
  }
  
  void MovePos(UINT32 aNumBits)
  {
    m_BitPos += aNumBits;
    Normalize();
  }
  
  UINT32 ReadBits(UINT32 aNumBits)
  {
    UINT32 aRes = GetValue(aNumBits);
    MovePos(aNumBits);
    return aRes;
  }
};

}}

#endif
