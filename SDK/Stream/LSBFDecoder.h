// Stream/LSBFDecoder.h

#pragma once

#ifndef __STREAM_LSBFDECODER_H
#define __STREAM_LSBFDECODER_H

#include "Interface/IInOutStreams.h"

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
  void Init(ISequentialInStream *aStream)
  {
    m_Stream.Init(aStream);
    Init();
  }
  void Init()
  {
    m_BitPos = kNumBigValueBits; 
    m_NormalValue = 0;
  }
  void ReleaseStream()
  {
    m_Stream.ReleaseStream();
  }
  UINT64 GetProcessedSize() const 
    { return m_Stream.GetProcessedSize() - (kNumBigValueBits - m_BitPos) / 8; }

  void Normalize()
  {
    for (;m_BitPos >= 8; m_BitPos -= 8)
    {
      BYTE aByte = m_Stream.ReadByte();
      m_NormalValue = (aByte << (kNumBigValueBits - m_BitPos)) | m_NormalValue;
      m_Value = (m_Value << 8) | kInvertTable[aByte];
    }
  }
  
  UINT32 GetValue(UINT32 aNumBits)
  {
    Normalize();
    return ((m_Value >> (8 - m_BitPos)) & kMask) >> (kNumValueBits - aNumBits);
  }

  void MovePos(UINT32 aNumBits)
  {
    m_BitPos += aNumBits;
    m_NormalValue >>= aNumBits;
  }
  
  UINT32 ReadBits(UINT32 aNumBits)
  {
    Normalize();
    UINT32 aRes = m_NormalValue & ( (1 << aNumBits) - 1);
    MovePos(aNumBits);
    return aRes;
  }
  
  UINT32 GetBitPosition() const
  {
    return (m_BitPos & 7);
  }
  
};

}}

#endif
