// Archive/Cab/LZXBitDecoder.h

#ifndef __ARCHIVE_CAB_LZXBITDECODER_H
#define __ARCHIVE_CAB_LZXBITDECODER_H

#include "CabInBuffer.h"

namespace NArchive {
namespace NCab {
namespace NLZX {
namespace NBitStream {

const int kNumBigValueBits = 8 * 4;

const int kNumValueBits = 17;
const int kBitDecoderValueMask = (1 << kNumValueBits) - 1;

class CDecoder
{
protected:
  CInBuffer m_Stream;
  UInt32 m_BitPos;
  UInt32 m_Value;
public:
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialInStream *s) { m_Stream.SetStream(s); }
  void ReleaseStream() { m_Stream.ReleaseStream(); }
  void Init(Byte reservedSize, UInt32 numBlocks)
  { 
    m_Stream.Init(reservedSize, numBlocks);
  }
  UInt64 GetProcessedSize() const 
    { return m_Stream.GetProcessedSize() - (kNumBigValueBits - m_BitPos) / 8; }
  UInt32 GetBitPosition() const
    { return UInt32(m_Stream.GetProcessedSize() * 8 - (kNumBigValueBits - m_BitPos)); }
  
  void Init()
  {
    m_BitPos = kNumBigValueBits; 
    Normalize();
  }

  void Normalize()
  {
    for (;m_BitPos >= 16; m_BitPos -= 16)
    {
      Byte b0 = m_Stream.ReadByte();
      Byte b1 = m_Stream.ReadByte();
      m_Value = (m_Value << 8) | b1;
      m_Value = (m_Value << 8) | b0;
    }
  }

  UInt32 GetValue(UInt32 numBits)
  {
    return ((m_Value >> ((32 - kNumValueBits) - m_BitPos)) & kBitDecoderValueMask) >> 
        (kNumValueBits - numBits);
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
  UInt32 ReadBitsBig(UInt32 numBits)
  {
    UInt32 numBits0 = numBits / 2;
    UInt32 numBits1 = numBits - numBits0;
    UInt32 res = ReadBits(numBits0) << numBits1;
    return res + ReadBits(numBits1);
  }

  Byte DirectReadByte()
  {
    if (m_BitPos == kNumBigValueBits)
      return m_Stream.ReadByte();
    Byte res;
    switch(m_BitPos)
    {
      case 0:
        res = Byte(m_Value >> 16);
        break;
      case 8:
        res = Byte(m_Value >> 24);
        break;
      case 16:
        res = Byte(m_Value);
        break;
      case 24:
        res = Byte(m_Value >> 8);
        break;
    }
    m_BitPos += 8;
    return res;
  }

  HRESULT ReadBlock(UInt32 &uncompressedSize, bool &dataAreCorrect)
    { return m_Stream.ReadBlock(uncompressedSize, dataAreCorrect); }
};


}}}}

#endif
