// Compress/HuffmanDecoder.h

#ifndef __COMPRESS_HUFFMANDECODER_H
#define __COMPRESS_HUFFMANDECODER_H

#include "../../../Common/Types.h"

namespace NCompress {
namespace NHuffman {

class CDecoderException{};

const UInt32 kValueTableBits = 9;

template <int kNumBitsInLongestCode, UInt32 m_NumSymbols>
class CDecoder
{
  UInt32 m_Limitits[kNumBitsInLongestCode + 1]; // m_Limitits[i] = value limit for symbols with length = i 
  UInt32 m_Positions[kNumBitsInLongestCode + 1];   // m_Positions[i] = index in m_Symbols[] of first symbol with length = i 
  UInt32 m_Symbols[m_NumSymbols]; // symbols: at first with len = 1 then 2, ... 15.
  Byte m_Lengths[1 << kValueTableBits];
public:
  void SetNumSymbols(UInt32 numSymbols) { m_NumSymbols = numSymbols; }
  void SetCodeLengths(const Byte *codeLengths);
  template <class TBitDecoder>
  UInt32 DecodeSymbol(TBitDecoder *bitStream)
  {
    UInt32 numBits;
    
    UInt32 value = bitStream->GetValue(kNumBitsInLongestCode);
    
    if (value < m_Limitits[kValueTableBits])
      numBits = m_Lengths[value >> (kNumBitsInLongestCode - kValueTableBits)];
    else 
      for (numBits = kValueTableBits + 1; numBits < kNumBitsInLongestCode; numBits++)
        if (value < m_Limitits[numBits])
          break;
    bitStream->MovePos(numBits);
    UInt32 index = m_Positions[numBits] + 
      ((value - m_Limitits[numBits - 1]) >> (kNumBitsInLongestCode - numBits));
    if (index >= m_NumSymbols)
      throw CDecoderException(); // test it
    return m_Symbols[index];
  }
};

template <int kNumBitsInLongestCode, UInt32 m_NumSymbols>
void CDecoder<kNumBitsInLongestCode, m_NumSymbols>::SetCodeLengths(const Byte *codeLengths)
{
  int lenCounts[kNumBitsInLongestCode + 1], tmpPositions[kNumBitsInLongestCode + 1];
  int i;
  for(i = 1; i <= kNumBitsInLongestCode; i++)
    lenCounts[i] = 0;
  UInt32 symbol;
  for (symbol = 0; symbol < m_NumSymbols; symbol++)
  {
    Byte codeLength = codeLengths[symbol];
    if (codeLength > kNumBitsInLongestCode)
      throw CDecoderException();
    lenCounts[codeLength]++;
  }
  lenCounts[0] = 0;
  tmpPositions[0] = m_Positions[0] = m_Limitits[0] = 0;
  UInt32 startPos = 0;
  UInt32 index = 0;
  const UInt32 kMaxValue = (1 << kNumBitsInLongestCode);
  for (i = 1; i <= kNumBitsInLongestCode; i++)
  {
    startPos += lenCounts[i] << (kNumBitsInLongestCode - i);
    if (startPos > kMaxValue)
      throw CDecoderException();
    m_Limitits[i] = startPos;
    m_Positions[i] = m_Positions[i - 1] + lenCounts[i - 1];
    tmpPositions[i] = m_Positions[i];

    if(i <= kValueTableBits)
    {
      UInt32 limit = (m_Limitits[i] >> (kNumBitsInLongestCode - kValueTableBits)); // change it
      memset(m_Lengths + index, Byte(i), limit - index);
      index = limit;
    }
  }

  // if (startPos != kMaxValue)
  //   throw CDecoderException();

  for (symbol = 0; symbol < m_NumSymbols; symbol++)
    if (codeLengths[symbol] != 0)
      m_Symbols[tmpPositions[codeLengths[symbol]]++] = symbol;
}

}}

#endif
