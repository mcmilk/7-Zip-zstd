// Implode/HuffmanDecoder.cpp

#include "StdAfx.h"

#include "HuffmanDecoder.h"

namespace NImplode {
namespace NHuffman {

CDecoder::CDecoder(UINT32 numSymbols):
  m_NumSymbols(numSymbols)
{
  m_Symbols = new UINT32[m_NumSymbols];
}

CDecoder::~CDecoder()
{
  delete []m_Symbols;
}

void CDecoder::SetCodeLengths(const BYTE *codeLengths)
{
  // int lenCounts[kNumBitsInLongestCode + 1], tmpPositions[kNumBitsInLongestCode + 1];
  int lenCounts[kNumBitsInLongestCode + 2], tmpPositions[kNumBitsInLongestCode + 1];
  int i;
  for(i = 0; i <= kNumBitsInLongestCode; i++)
    lenCounts[i] = 0;
  UINT32 symbolIndex;
  for (symbolIndex = 0; symbolIndex < m_NumSymbols; symbolIndex++)
    lenCounts[codeLengths[symbolIndex]]++;
  // lenCounts[0] = 0;
  
  // tmpPositions[0] = m_Positions[0] = m_Limitits[0] = 0;
  m_Limitits[kNumBitsInLongestCode + 1] = 0;
  m_Positions[kNumBitsInLongestCode + 1] = 0;
  lenCounts[kNumBitsInLongestCode + 1] =  0;


  UINT32 startPos = 0;
  static const UINT32 kMaxValue = (1 << kNumBitsInLongestCode);

  for (i = kNumBitsInLongestCode; i > 0; i--)
  {
    startPos += lenCounts[i] << (kNumBitsInLongestCode - i);
    if (startPos > kMaxValue)
      throw CDecoderException();
    m_Limitits[i] = startPos;
    m_Positions[i] = m_Positions[i + 1] + lenCounts[i + 1];
    tmpPositions[i] = m_Positions[i] + lenCounts[i];

  }

  // if _ZIP_MODE do not throw exception for trees containing only one node 
  // #ifndef _ZIP_MODE 
  if (startPos != kMaxValue)
    throw CDecoderException();
  // #endif

  for (symbolIndex = 0; symbolIndex < m_NumSymbols; symbolIndex++)
    if (codeLengths[symbolIndex] != 0)
      m_Symbols[--tmpPositions[codeLengths[symbolIndex]]] = symbolIndex;
}

UINT32 CDecoder::DecodeSymbol(CInBit *inStream)
{
  UINT32 numBits;
  UINT32 value = inStream->GetValue(kNumBitsInLongestCode);
  int i;
  for(i = kNumBitsInLongestCode; i > 0; i--)
  {
    if (value < m_Limitits[i])
    {
      numBits = i;
      break;
    }
  }
  if (i == 0)
    throw CDecoderException();
  inStream->MovePos(numBits);
  UINT32 index = m_Positions[numBits] + 
      ((value - m_Limitits[numBits + 1]) >> (kNumBitsInLongestCode - numBits));
  if (index >= m_NumSymbols)
    throw CDecoderException(); // test it
  return m_Symbols[index];
}

}}
