// Compression/HuffmanDecoder.h

#pragma once

#ifndef __COMPRESSION_HUFFMANDECODER_H
#define __COMPRESSION_HUFFMANDECODER_H

namespace NCompression {
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
  CDecoder(UINT32 aNumSymbols):
    m_NumSymbols(aNumSymbols)
    { m_Symbols = new UINT32[m_NumSymbols]; }

  ~CDecoder()
    { delete []m_Symbols; }

  void SetNumSymbols(UINT32 aNumSymbols)
    { m_NumSymbols = aNumSymbols; }
  void SetCodeLengths(const BYTE *aCodeLengths);
  template <class TBitDecoder>
  UINT32 DecodeSymbol(TBitDecoder *aStream)
  {
    UINT32 aNumBits;
    
    UINT32 aValue = aStream->GetValue(kNumBitsInLongestCode);
    
    if (aValue < m_Limitits[kValueTableBits])
      aNumBits = m_Lengths[aValue >> (kNumBitsInLongestCode - kValueTableBits)];
    else if (aValue < m_Limitits[10])
      if (aValue < m_Limitits[9])
        aNumBits = 9;
      else
        aNumBits = 10;
    else if (aValue < m_Limitits[11])
      aNumBits = 11;
    else if (aValue < m_Limitits[12])
      aNumBits = 12;
    else 
      for (aNumBits = 13; aNumBits < kNumBitsInLongestCode; aNumBits++)
        if (aValue < m_Limitits[aNumBits])
          break;
    aStream->MovePos(aNumBits);
    UINT32 anIndex = m_Positions[aNumBits] + 
      ((aValue - m_Limitits[aNumBits - 1]) >> (kNumBitsInLongestCode - aNumBits));
    if (anIndex >= m_NumSymbols)
      throw CDecoderException(); // test it
    return m_Symbols[anIndex];
  }
};


template <int kNumBitsInLongestCode>
void CDecoder<kNumBitsInLongestCode>::SetCodeLengths(const BYTE *aCodeLengths)
{
  int aLenCounts[kNumBitsInLongestCode + 1], aTmpPositions[kNumBitsInLongestCode + 1];
  int i;
  for(i = 1; i <= kNumBitsInLongestCode; i++)
    aLenCounts[i] = 0;
  UINT32 aSymbol;
  for (aSymbol = 0; aSymbol < m_NumSymbols; aSymbol++)
  {
    BYTE aCodeLength = aCodeLengths[aSymbol];
    if (aCodeLength > kNumBitsInLongestCode)
      throw CDecoderException();
    aLenCounts[aCodeLength]++;
  }
  aLenCounts[0] = 0;
  aTmpPositions[0] = m_Positions[0] = m_Limitits[0] = 0;
  UINT32 aStartPos = 0;
  UINT32 anIndex = 0;
  const kMaxValue = (1 << kNumBitsInLongestCode);
  for (i = 1; i <= kNumBitsInLongestCode; i++)
  {
    aStartPos += aLenCounts[i] << (kNumBitsInLongestCode - i);
    if (aStartPos > kMaxValue)
      throw CDecoderException();
    m_Limitits[i] = aStartPos;
    m_Positions[i] = m_Positions[i - 1] + aLenCounts[i - 1];
    aTmpPositions[i] = m_Positions[i];

    if(i <= kValueTableBits)
    {
      UINT32 aLimit = (m_Limitits[i] >> (kNumBitsInLongestCode - kValueTableBits)); // change it
      memset(m_Lengths + anIndex, BYTE(i), aLimit - anIndex);
      anIndex = aLimit;
    }
  }

  // if (aStartPos != kMaxValue)
  //   throw CDecoderException();

  for (aSymbol = 0; aSymbol < m_NumSymbols; aSymbol++)
    if (aCodeLengths[aSymbol] != 0)
      m_Symbols[aTmpPositions[aCodeLengths[aSymbol]]++] = aSymbol;
}

}}

#endif
