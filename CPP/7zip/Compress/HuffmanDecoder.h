// Compress/HuffmanDecoder.h

#ifndef __COMPRESS_HUFFMAN_DECODER_H
#define __COMPRESS_HUFFMAN_DECODER_H

#include "../../Common/Types.h"

namespace NCompress {
namespace NHuffman {

const int kNumTableBits = 9;

template <int kNumBitsMax, UInt32 m_NumSymbols>
class CDecoder
{
  UInt32 m_Limits[kNumBitsMax + 1];     // m_Limits[i] = value limit for symbols with length = i
  UInt32 m_Positions[kNumBitsMax + 1];  // m_Positions[i] = index in m_Symbols[] of first symbol with length = i
  UInt32 m_Symbols[m_NumSymbols];
  Byte m_Lengths[1 << kNumTableBits];   // Table oh length for short codes.

public:
  
  bool SetCodeLengths(const Byte *codeLengths)
  {
    int lenCounts[kNumBitsMax + 1];
    UInt32 tmpPositions[kNumBitsMax + 1];
    int i;
    for(i = 1; i <= kNumBitsMax; i++)
      lenCounts[i] = 0;
    UInt32 symbol;
    for (symbol = 0; symbol < m_NumSymbols; symbol++)
    {
      int len = codeLengths[symbol];
      if (len > kNumBitsMax)
        return false;
      lenCounts[len]++;
      m_Symbols[symbol] = 0xFFFFFFFF;
    }
    lenCounts[0] = 0;
    m_Positions[0] = m_Limits[0] = 0;
    UInt32 startPos = 0;
    UInt32 index = 0;
    const UInt32 kMaxValue = (1 << kNumBitsMax);
    for (i = 1; i <= kNumBitsMax; i++)
    {
      startPos += lenCounts[i] << (kNumBitsMax - i);
      if (startPos > kMaxValue)
        return false;
      m_Limits[i] = (i == kNumBitsMax) ? kMaxValue : startPos;
      m_Positions[i] = m_Positions[i - 1] + lenCounts[i - 1];
      tmpPositions[i] = m_Positions[i];
      if(i <= kNumTableBits)
      {
        UInt32 limit = (m_Limits[i] >> (kNumBitsMax - kNumTableBits));
        for (; index < limit; index++)
          m_Lengths[index] = (Byte)i;
      }
    }
    for (symbol = 0; symbol < m_NumSymbols; symbol++)
    {
      int len = codeLengths[symbol];
      if (len != 0)
        m_Symbols[tmpPositions[len]++] = symbol;
    }
    return true;
  }

  template <class TBitDecoder>
  UInt32 DecodeSymbol(TBitDecoder *bitStream)
  {
    int numBits;
    UInt32 value = bitStream->GetValue(kNumBitsMax);
    if (value < m_Limits[kNumTableBits])
      numBits = m_Lengths[value >> (kNumBitsMax - kNumTableBits)];
    else
      for (numBits = kNumTableBits + 1; value >= m_Limits[numBits]; numBits++);
    bitStream->MovePos(numBits);
    UInt32 index = m_Positions[numBits] +
      ((value - m_Limits[numBits - 1]) >> (kNumBitsMax - numBits));
    if (index >= m_NumSymbols)
      // throw CDecoderException(); // test it
      return 0xFFFFFFFF;
    return m_Symbols[index];
  }
};

}}

#endif
