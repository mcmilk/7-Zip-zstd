// Compress/HuffmanDecoder.h

#pragma once

#ifndef __COMPRESS_HUFFMANDECODER_H
#define __COMPRESS_HUFFMANDECODER_H

namespace NCompress {
namespace NHuffman {

class CDecoderException{};

const UINT32 kValueTableBits = 8;

template <int kNumBitsInLongestCode>
class CDecoder
{
  UINT32 m_Limitits[kNumBitsInLongestCode + 1]; // m_Limitits[i] = value limit for symbols with length = i 
  UINT32 m_Positions[kNumBitsInLongestCode + 1];   // m_Positions[i] = index in m_Symbols[] of first symbol with length = i 
  UINT32 m_NumSymbols;
  UINT32 *m_Symbols; // symbols: at first with len = 1 then 2, ... 15.
  BYTE m_Lengths[1 << kValueTableBits];
public:
  CDecoder(UINT32 numSymbols):
    m_NumSymbols(numSymbols)
    { m_Symbols = new UINT32[m_NumSymbols]; }

  ~CDecoder()
    { delete []m_Symbols; }

  void SetNumSymbols(UINT32 numSymbols)
    { m_NumSymbols = numSymbols; }
  void SetCodeLengths(const BYTE *codeLengths);
  template <class TBitDecoder>
  UINT32 DecodeSymbol(TBitDecoder *bitStream)
  {
    UINT32 numBits;
    
    UINT32 value = bitStream->GetValue(kNumBitsInLongestCode);
    
    if (value < m_Limitits[kValueTableBits])
      numBits = m_Lengths[value >> (kNumBitsInLongestCode - kValueTableBits)];
    else if (value < m_Limitits[10])
      if (value < m_Limitits[9])
        numBits = 9;
      else
        numBits = 10;
    else if (value < m_Limitits[11])
      numBits = 11;
    else if (value < m_Limitits[12])
      numBits = 12;
    else 
      for (numBits = 13; numBits < kNumBitsInLongestCode; numBits++)
        if (value < m_Limitits[numBits])
          break;
    bitStream->MovePos(numBits);
    UINT32 index = m_Positions[numBits] + 
      ((value - m_Limitits[numBits - 1]) >> (kNumBitsInLongestCode - numBits));
    if (index >= m_NumSymbols)
      throw CDecoderException(); // test it
    return m_Symbols[index];
  }
};


template <int kNumBitsInLongestCode>
void CDecoder<kNumBitsInLongestCode>::SetCodeLengths(const BYTE *codeLengths)
{
  int lenCounts[kNumBitsInLongestCode + 1], tmpPositions[kNumBitsInLongestCode + 1];
  int i;
  for(i = 1; i <= kNumBitsInLongestCode; i++)
    lenCounts[i] = 0;
  UINT32 symbol;
  for (symbol = 0; symbol < m_NumSymbols; symbol++)
  {
    BYTE codeLength = codeLengths[symbol];
    if (codeLength > kNumBitsInLongestCode)
      throw CDecoderException();
    lenCounts[codeLength]++;
  }
  lenCounts[0] = 0;
  tmpPositions[0] = m_Positions[0] = m_Limitits[0] = 0;
  UINT32 startPos = 0;
  UINT32 index = 0;
  const kMaxValue = (1 << kNumBitsInLongestCode);
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
      UINT32 limit = (m_Limitits[i] >> (kNumBitsInLongestCode - kValueTableBits)); // change it
      memset(m_Lengths + index, BYTE(i), limit - index);
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
