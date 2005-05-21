// Compression/HuffmanEncoder.h

#ifndef __COMPRESSION_HUFFMANENCODER_H
#define __COMPRESSION_HUFFMANENCODER_H

#include "../../../Common/Types.h"

namespace NCompression {
namespace NHuffman {

const int kNumBitsInLongestCode = 20;

struct CItem
{
  UInt32 Freq;
  UInt32 Code;
  UInt32 Dad;
  UInt32 Len;
};

class CEncoder
{
public:
  UInt32 m_NumSymbols; // number of symbols in adwSymbol

  CItem *m_Items;
  UInt32 *m_Heap;
  UInt32 m_HeapSize;
  Byte *m_Depth;
  const Byte *m_ExtraBits;
  UInt32 m_ExtraBase;
  UInt32 m_MaxLength;

  UInt32 m_HeapLength;
  UInt32 m_BitLenCounters[kNumBitsInLongestCode + 1];

  UInt32 RemoveSmallest();
  bool Smaller(int n, int m); 
  void DownHeap(UInt32 k);
  void GenerateBitLen(UInt32 maxCode, UInt32 heapMax);
  void GenerateCodes(UInt32 maxCode);
  
  UInt32 m_BlockBitLength;

  void Free();

public:

  CEncoder();
  ~CEncoder();
  bool Create(UInt32 numSymbols, const Byte *extraBits, 
      UInt32 extraBase, UInt32 maxLength);
  void StartNewBlock();

  void AddSymbol(UInt32 symbol) {  m_Items[symbol].Freq++; }

  void SetFreqs(const UInt32 *freqs);
  void BuildTree(Byte *levels);
  UInt32 GetBlockBitLength() const { return m_BlockBitLength; }

  template <class TBitEncoder>
  void CodeOneValue(TBitEncoder *bitEncoder, UInt32 symbol)
    { bitEncoder->WriteBits(m_Items[symbol].Code, m_Items[symbol].Len); }
};

}}

#endif
