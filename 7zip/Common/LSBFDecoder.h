// Stream/LSBFDecoder.h

#pragma once

#ifndef __STREAM_LSBFDECODER_H
#define __STREAM_LSBFDECODER_H

#include "../IStream.h"

namespace NStream {
namespace NLSBF {

const kNumBigValueBits = 8 * 4;

const kNumValueBytes = 3;
const kNumValueBits = 8  * kNumValueBytes;

const kMask = (1 << kNumValueBits) - 1;

extern BYTE kInvertTable[256];
// the Least Significant Bit of byte is First

template<class TInByte>
class CDecoder
{
  UINT32 m_BitPos;
  UINT32 m_Value;
  UINT32 m_NormalValue;
protected:
  TInByte m_Stream;
public:
  UINT32 NumExtraBytes;
  void Init(ISequentialInStream *inStream)
  {
    m_Stream.Init(inStream);
    Init();
  }
  void Init()
  {
    m_BitPos = kNumBigValueBits; 
    m_NormalValue = 0;
    NumExtraBytes = 0;
  }
  /*
  void ReleaseStream()
  {
    m_Stream.ReleaseStream();
  }
  */
  UINT64 GetProcessedSize() const 
    { return m_Stream.GetProcessedSize() - (kNumBigValueBits - m_BitPos) / 8; }
  UINT64 GetProcessedBitsSize() const 
    { return (m_Stream.GetProcessedSize() << 3) - (kNumBigValueBits - m_BitPos); }

  void Normalize()
  {
    for (;m_BitPos >= 8; m_BitPos -= 8)
    {
      BYTE b;
      if (!m_Stream.ReadByte(b))
        NumExtraBytes++;
      m_NormalValue = (b << (kNumBigValueBits - m_BitPos)) | m_NormalValue;
      m_Value = (m_Value << 8) | kInvertTable[b];
    }
  }
  
  UINT32 GetValue(UINT32 numBits)
  {
    Normalize();
    return ((m_Value >> (8 - m_BitPos)) & kMask) >> (kNumValueBits - numBits);
  }

  void MovePos(UINT32 numBits)
  {
    m_BitPos += numBits;
    m_NormalValue >>= numBits;
  }
  
  UINT32 ReadBits(UINT32 numBits)
  {
    Normalize();
    UINT32 res = m_NormalValue & ( (1 << numBits) - 1);
    MovePos(numBits);
    return res;
  }
  
  UINT32 GetBitPosition() const
  {
    return (m_BitPos & 7);
  }
  
};

}}

#endif
