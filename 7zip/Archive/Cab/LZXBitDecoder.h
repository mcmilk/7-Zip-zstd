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
  UINT32 m_BitPos;
  UINT32 m_Value;
public:
  void InitStream(ISequentialInStream *aStream, BYTE aReservedSize, UINT32 aNumBlocks)
    { 
      m_Stream.Init(aStream, aReservedSize, aNumBlocks);
    }
  /*
  void ReleaseStream()
    { m_Stream.ReleaseStream(); }
  */
  UINT64 GetProcessedSize() const 
    { return m_Stream.GetProcessedSize() - (kNumBigValueBits - m_BitPos) / 8; }
  UINT32 GetBitPosition() const
    { return UINT32(m_Stream.GetProcessedSize() * 8 - (kNumBigValueBits - m_BitPos)); }
  
  void Init()
  {
    m_BitPos = kNumBigValueBits; 
    Normalize();
  }

  void Normalize()
  {
    for (;m_BitPos >= 16; m_BitPos -= 16)
    {
      BYTE aByte0 = m_Stream.ReadByte();
      BYTE aByte1 = m_Stream.ReadByte();
      m_Value = (m_Value << 8) | aByte1;
      m_Value = (m_Value << 8) | aByte0;
    }
  }

  UINT32 GetValue(UINT32 aNumBits)
  {
    return ((m_Value >> ((32 - kNumValueBits) - m_BitPos)) & kBitDecoderValueMask) >> 
        (kNumValueBits - aNumBits);
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
  UINT32 ReadBitsBig(UINT32 aNumBits)
  {
    UINT32 aNumBits0 = aNumBits / 2;
    UINT32 aNumBits1 = aNumBits - aNumBits0;
    UINT32 aRes = ReadBits(aNumBits0) << aNumBits1;
    return aRes + ReadBits(aNumBits1);
  }

  BYTE DirectReadByte()
  {
    if (m_BitPos == kNumBigValueBits)
      return m_Stream.ReadByte();
    BYTE aRes;
    switch(m_BitPos)
    {
      case 0:
        aRes = BYTE(m_Value >> 16);
        break;
      case 8:
        aRes = BYTE(m_Value >> 24);
        break;
      case 16:
        aRes = BYTE(m_Value);
        break;
      case 24:
        aRes = BYTE(m_Value >> 8);
        break;
    }
    m_BitPos += 8;
    return aRes;
  }

  HRESULT ReadBlock(UINT32 &anUncompressedSize, bool &aDataAreCorrect)
    { return m_Stream.ReadBlock(anUncompressedSize, aDataAreCorrect); }
};


}}}}

#endif
