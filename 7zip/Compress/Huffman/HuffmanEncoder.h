// Compression/HuffmanEncoder.h

#pragma once

#ifndef __COMPRESSION_HUFFMANENCODER_H
#define __COMPRESSION_HUFFMANENCODER_H

#include "../../../Common/Types.h"

namespace NCompression {
namespace NHuffman {

const int kNumBitsInLongestCode = 15;

struct CItem
{
  UINT32 Freq;
  UINT32 Code;
  UINT32 Dad;
  UINT32 Len;
};

class CEncoder
{
  UINT32 m_NumSymbols; // number of symbols in adwSymbol

  CItem *m_Items;
  UINT32 *m_Heap;
  UINT32 m_HeapSize;
  BYTE *m_Depth;
  const BYTE *m_ExtraBits;
  UINT32 m_ExtraBase;
  UINT32 m_MaxLength;

  UINT32 m_HeapLength;
  UINT32 m_BitLenCounters[kNumBitsInLongestCode + 1];

  UINT32 RemoveSmallest();
  bool Smaller(int n, int m); 
  void DownHeap(UINT32 k);
  void GenerateBitLen(UINT32 maxCode, UINT32 heapMax);
  void GenerateCodes(UINT32 maxCode);
  
  UINT32 m_BlockBitLength;
public:

  CEncoder(UINT32 numSymbols, const BYTE *extraBits, 
      UINT32 extraBase, UINT32 maxLength);
  ~CEncoder();
  void StartNewBlock();

  void AddSymbol(UINT32 symbol)
    {  m_Items[symbol].Freq++; }

  void SetFreqs(const UINT32 *freqs);
  void BuildTree(BYTE *levels);
  UINT32 GetBlockBitLength() const { return m_BlockBitLength; }

  template <class TBitEncoder>
  void CodeOneValue(TBitEncoder *bitEncoder, UINT32 symbol)
    { bitEncoder->WriteBits(m_Items[symbol].Code, m_Items[symbol].Len); }
};

}}

#endif