// Implode/HuffmanDecoder.h

#pragma once

#ifndef __IMPLODE_HUFFMANDECODER_H
#define __IMPLODE_HUFFMANDECODER_H

#include "Stream/LSBFDecoder.h"
#include "Stream/InByte.h"

namespace NImplode {
namespace NHuffman {

  static const kNumBitsInLongestCode = 16;
  class CDecoderException{};
  
  typedef NStream::NLSBF::CDecoder<NStream::CInByte> CInBit;

  class CDecoder
  {
    UINT32 m_Limitits[kNumBitsInLongestCode + 2]; // m_Limitits[i] = value limit for symbols with length = i 
    UINT32 m_Positions[kNumBitsInLongestCode + 2];   // m_Positions[i] = index in m_Symbols[] of first symbol with length = i 
    UINT32 m_NumSymbols; // number of symbols in m_Symbols
    UINT32 *m_Symbols; // symbols: at first with len=1 then 2, ... 15.
  public:
    CDecoder(UINT32 aNumSymbols);
    ~CDecoder();
    
    void SetCodeLengths(const BYTE *aCodeLengths);
    UINT32 DecodeSymbol(CInBit *anInStream);
  };

}}

#endif
