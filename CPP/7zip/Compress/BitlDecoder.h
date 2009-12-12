// BitlDecoder.h -- the Least Significant Bit of byte is First

#ifndef __BITL_DECODER_H
#define __BITL_DECODER_H

#include "../IStream.h"

namespace NBitl {

const unsigned kNumBigValueBits = 8 * 4;
const unsigned kNumValueBytes = 3;
const unsigned kNumValueBits = 8  * kNumValueBytes;

const UInt32 kMask = (1 << kNumValueBits) - 1;

extern Byte kInvertTable[256];

template<class TInByte>
class CBaseDecoder
{
protected:
  unsigned m_BitPos;
  UInt32 m_Value;
  TInByte m_Stream;
public:
  UInt32 NumExtraBytes;
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialInStream *inStream) { m_Stream.SetStream(inStream); }
  void ReleaseStream() { m_Stream.ReleaseStream(); }
  void Init()
  {
    m_Stream.Init();
    m_BitPos = kNumBigValueBits;
    m_Value = 0;
    NumExtraBytes = 0;
  }
  UInt64 GetProcessedSize() const { return m_Stream.GetProcessedSize() + NumExtraBytes - (kNumBigValueBits - m_BitPos) / 8; }

  void Normalize()
  {
    for (; m_BitPos >= 8; m_BitPos -= 8)
    {
      Byte b = 0;
      if (!m_Stream.ReadByte(b))
      {
        b = 0xFF; // check it
        NumExtraBytes++;
      }
      m_Value = (b << (kNumBigValueBits - m_BitPos)) | m_Value;
    }
  }
  
  UInt32 ReadBits(unsigned numBits)
  {
    Normalize();
    UInt32 res = m_Value & ((1 << numBits) - 1);
    m_BitPos += numBits;
    m_Value >>= numBits;
    return res;
  }

  bool ExtraBitsWereRead() const
  {
    if (NumExtraBytes == 0)
      return false;
    return ((UInt32)(kNumBigValueBits - m_BitPos) < (NumExtraBytes << 3));
  }
};

template<class TInByte>
class CDecoder: public CBaseDecoder<TInByte>
{
  UInt32 m_NormalValue;

public:
  void Init()
  {
    CBaseDecoder<TInByte>::Init();
    m_NormalValue = 0;
  }

  void Normalize()
  {
    for (; this->m_BitPos >= 8; this->m_BitPos -= 8)
    {
      Byte b = 0;
      if (!this->m_Stream.ReadByte(b))
      {
        b = 0xFF; // check it
        this->NumExtraBytes++;
      }
      m_NormalValue = (b << (kNumBigValueBits - this->m_BitPos)) | m_NormalValue;
      this->m_Value = (this->m_Value << 8) | kInvertTable[b];
    }
  }
  
  UInt32 GetValue(unsigned numBits)
  {
    Normalize();
    return ((this->m_Value >> (8 - this->m_BitPos)) & kMask) >> (kNumValueBits - numBits);
  }

  void MovePos(unsigned numBits)
  {
    this->m_BitPos += numBits;
    m_NormalValue >>= numBits;
  }
  
  UInt32 ReadBits(unsigned numBits)
  {
    Normalize();
    UInt32 res = m_NormalValue & ((1 << numBits) - 1);
    MovePos(numBits);
    return res;
  }

  void AlignToByte() { MovePos((32 - this->m_BitPos) & 7); }

  Byte ReadByte()
  {
    if (this->m_BitPos == kNumBigValueBits)
    {
      Byte b = 0;
      if (!this->m_Stream.ReadByte(b))
      {
        b = 0xFF;
        this->NumExtraBytes++;
      }
      return b;
    }
    {
      Byte b = (Byte)(m_NormalValue & 0xFF);
      MovePos(8);
      return b;
    }
  }
};

}

#endif
