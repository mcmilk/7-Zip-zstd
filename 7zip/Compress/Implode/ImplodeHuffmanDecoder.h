// ImplodeHuffmanDecoder.h

#pragma once

#ifndef __IMPLODE_HUFFMAN_DECODER_H
#define __IMPLODE_HUFFMAN_DECODER_H

#include "../../Common/LSBFDecoder.h"
#include "../../Common/InBuffer.h"

namespace NCompress {
namespace NImplode {
namespace NHuffman {

const int kNumBitsInLongestCode = 16;
class CDecoderException{};

typedef NStream::NLSBF::CDecoder<CInBuffer> CInBit;

class CDecoder
{
  UINT32 m_Limitits[kNumBitsInLongestCode + 2]; // m_Limitits[i] = value limit for symbols with length = i 
  UINT32 m_Positions[kNumBitsInLongestCode + 2];   // m_Positions[i] = index in m_Symbols[] of first symbol with length = i 
  UINT32 m_NumSymbols; // number of symbols in m_Symbols
  UINT32 *m_Symbols; // symbols: at first with len=1 then 2, ... 15.
public:
  CDecoder(UINT32 numSymbols);
  ~CDecoder();
  
  void SetCodeLengths(const BYTE *codeLengths);
  UINT32 DecodeSymbol(CInBit *inStream);
};

}}}

#endif
