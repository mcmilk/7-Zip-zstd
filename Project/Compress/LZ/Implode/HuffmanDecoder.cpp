// Implode/HuffmanDecoder.cpp

#include "StdAfx.h"

#include "HuffmanDecoder.h"

namespace NImplode {
namespace NHuffman {

CDecoder::CDecoder(UINT32 aNumSymbols):
  m_NumSymbols(aNumSymbols)
{
  m_Symbols = new UINT32[m_NumSymbols];
}

CDecoder::~CDecoder()
{
  delete []m_Symbols;
}

void CDecoder::SetCodeLengths(const BYTE *aCodeLengths)
{
  // int aLenCounts[kNumBitsInLongestCode + 1], aTmpPositions[kNumBitsInLongestCode + 1];
  int aLenCounts[kNumBitsInLongestCode + 2], aTmpPositions[kNumBitsInLongestCode + 1];
  int i;
  for(i = 0; i <= kNumBitsInLongestCode; i++)
    aLenCounts[i] = 0;
  UINT32 aSymbolIndex;
  for (aSymbolIndex = 0; aSymbolIndex < m_NumSymbols; aSymbolIndex++)
    aLenCounts[aCodeLengths[aSymbolIndex]]++;
  // aLenCounts[0] = 0;
  
  // aTmpPositions[0] = m_Positions[0] = m_Limitits[0] = 0;
  m_Limitits[kNumBitsInLongestCode + 1] = 0;
  m_Positions[kNumBitsInLongestCode + 1] = 0;
  aLenCounts[kNumBitsInLongestCode + 1] =  0;


  UINT32 aStartPos = 0;
  // UINT32 anIndex = 0;
  static const kMaxValue = (1 << kNumBitsInLongestCode);

  for (i = kNumBitsInLongestCode; i > 0; i--)
  {
    aStartPos += aLenCounts[i] << (kNumBitsInLongestCode - i);
    if (aStartPos > kMaxValue)
      throw CDecoderException();
    m_Limitits[i] = aStartPos;
    m_Positions[i] = m_Positions[i + 1] + aLenCounts[i + 1];
    aTmpPositions[i] = m_Positions[i] + aLenCounts[i];

  }

  // if _ZIP_MODE do not throw exception for trees containing only one node 
  // #ifndef _ZIP_MODE 
  if (aStartPos != kMaxValue)
    throw CDecoderException();
  // #endif

  for (aSymbolIndex = 0; aSymbolIndex < m_NumSymbols; aSymbolIndex++)
    if (aCodeLengths[aSymbolIndex] != 0)
      m_Symbols[--aTmpPositions[aCodeLengths[aSymbolIndex]]] = aSymbolIndex;
}

UINT32 CDecoder::DecodeSymbol(CInBit *anInStream)
{
  UINT32 aNumBits;
  UINT32 aValue = anInStream->GetValue(kNumBitsInLongestCode);

  int i;
  for(i = kNumBitsInLongestCode; i > 0; i--)
  {
    if (aValue < m_Limitits[i])
    {
      aNumBits = i;
      break;
    }
  }
  if (i == 0)
    throw CDecoderException();
  anInStream->MovePos(aNumBits);
  UINT32 anIndex = m_Positions[aNumBits] + 
      ((aValue - m_Limitits[aNumBits + 1]) >> (kNumBitsInLongestCode - aNumBits));
  if (anIndex >= m_NumSymbols)
    throw CDecoderException(); // test it
  return m_Symbols[anIndex];
}

}}
