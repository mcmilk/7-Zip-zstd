// Compression/AriBitCoder.h

#pragma once

#ifndef __COMPRESSION_BITCODER_H
#define __COMPRESSION_BITCODER_H

#include "Common/Types.h"
#include "Compression/RangeCoder.h"

namespace NCompression {
namespace NArithmetic {

const kNumBitModelTotalBits  = 11;
const UINT32 kBitModelTotal = (1 << kNumBitModelTotalBits);

const kNumMoveReducingBits = 2;


class CPriceTables
{
public:
  UINT32 m_StatePrices[kBitModelTotal >> kNumMoveReducingBits];
  CPriceTables();
};

extern CPriceTables g_PriceTables;


/////////////////////////////
// CBitModel

template <int aNumMoveBits>
class CBitModel
{
public:
  UINT32 m_Probability;
  void UpdateModel(UINT32 aSymbol)
  {
    /*
    m_Probability -= (m_Probability + ((aSymbol - 1) & ((1 << aNumMoveBits) - 1))) >> aNumMoveBits;
    m_Probability += (1 - aSymbol) << (kNumBitModelTotalBits - aNumMoveBits);
    */
    if (aSymbol == 0)
      m_Probability += (kBitModelTotal - m_Probability) >> aNumMoveBits;
    else
      m_Probability -= (m_Probability) >> aNumMoveBits;
  }
public:
  void Init() { m_Probability = kBitModelTotal / 2; }
};

template <int aNumMoveBits>
class CBitEncoder: public CBitModel<aNumMoveBits>
{
public:
  void Encode(CRangeEncoder *aRangeEncoder, UINT32 aSymbol)
  {
    aRangeEncoder->EncodeBit(m_Probability, kNumBitModelTotalBits, aSymbol);
    UpdateModel(aSymbol);
  }
  UINT32 GetPrice(UINT32 aSymbol) const
  {
    return g_PriceTables.m_StatePrices[
      (((m_Probability - aSymbol) ^ ((-(int)aSymbol))) & (kBitModelTotal - 1)) >> kNumMoveReducingBits];
  }
};


template <int aNumMoveBits>
class CBitDecoder: public CBitModel<aNumMoveBits>
{
public:
  UINT32 Decode(CRangeDecoder *aRangeDecoder)
  {
    UINT32 aNewBound = (aRangeDecoder->m_Range >> kNumBitModelTotalBits) * m_Probability;
    if (aRangeDecoder->m_Code < aNewBound)
    {
      aRangeDecoder->m_Range = aNewBound;
      m_Probability += (kBitModelTotal - m_Probability) >> aNumMoveBits;
      if (aRangeDecoder->m_Range < kTopValue)
      {
        aRangeDecoder->m_Code = (aRangeDecoder->m_Code << 8) | aRangeDecoder->m_Stream.ReadByte();
        aRangeDecoder->m_Range <<= 8;
      }
      return 0;
    }
    else
    {
      aRangeDecoder->m_Range -= aNewBound;
      aRangeDecoder->m_Code -= aNewBound;
      m_Probability -= (m_Probability) >> aNumMoveBits;
      if (aRangeDecoder->m_Range < kTopValue)
      {
        aRangeDecoder->m_Code = (aRangeDecoder->m_Code << 8) | aRangeDecoder->m_Stream.ReadByte();
        aRangeDecoder->m_Range <<= 8;
      }
      return 1;
    }
  }
};

}}


#endif
