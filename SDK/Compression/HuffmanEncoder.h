// Compression/HuffmanEncoder.h

#pragma once

#ifndef __COMPRESSION_HUFFMANENCODER_H
#define __COMPRESSION_HUFFMANENCODER_H

namespace NCompression {
namespace NHuffman {

const kNumBitsInLongestCode = 15;

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
  void GenerateBitLen(UINT32 aMaxCode, UINT32 aHeapMax);
  void GenerateCodes(UINT32 aMaxCode);
  
  UINT32 m_BlockBitLength;
public:

  CEncoder(UINT32 aNumSymbols, const BYTE *anExtraBits, 
      UINT32 anExtraBase, UINT32 aMaxLength);
  ~CEncoder();
  void StartNewBlock();

  void AddSymbol(UINT32 aSymbol)
    {  m_Items[aSymbol].Freq++; }

  void SetFreqs(const UINT32 *aFreqs);
  void BuildTree(BYTE *aLevels);
  DWORD GetBlockBitLength() const { return m_BlockBitLength; }

  template <class TBitEncoder>
  void CodeOneValue(TBitEncoder *aStream, UINT32 aSymbol)
    { aStream->WriteBits(m_Items[aSymbol].Code, m_Items[aSymbol].Len); }
};

}}

#endif