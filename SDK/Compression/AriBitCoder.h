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
  UINT32 StatePrices[kBitModelTotal >> kNumMoveReducingBits];
  CPriceTables();
};

extern CPriceTables g_PriceTables;


/////////////////////////////
// CBitModel

template <int aNumMoveBits>
class CBitModel
{
public:
  UINT32 Probability;
  void UpdateModel(UINT32 symbol)
  {
    /*
    Probability -= (Probability + ((symbol - 1) & ((1 << aNumMoveBits) - 1))) >> aNumMoveBits;
    Probability += (1 - symbol) << (kNumBitModelTotalBits - aNumMoveBits);
    */
    if (symbol == 0)
      Probability += (kBitModelTotal - Probability) >> aNumMoveBits;
    else
      Probability -= (Probability) >> aNumMoveBits;
  }
public:
  void Init() { Probability = kBitModelTotal / 2; }
};

template <int aNumMoveBits>
class CBitEncoder: public CBitModel<aNumMoveBits>
{
public:
  void Encode(CRangeEncoder *rangeEncoder, UINT32 symbol)
  {
    rangeEncoder->EncodeBit(Probability, kNumBitModelTotalBits, symbol);
    UpdateModel(symbol);
  }
  UINT32 GetPrice(UINT32 symbol) const
  {
    return g_PriceTables.StatePrices[
      (((Probability - symbol) ^ ((-(int)symbol))) & (kBitModelTotal - 1)) >> kNumMoveReducingBits];
  }
};


template <int aNumMoveBits>
class CBitDecoder: public CBitModel<aNumMoveBits>
{
public:
  UINT32 Decode(CRangeDecoder *rangeDecoder)
  {
    UINT32 newBound = (rangeDecoder->Range >> kNumBitModelTotalBits) * Probability;
    if (rangeDecoder->Code < newBound)
    {
      rangeDecoder->Range = newBound;
      Probability += (kBitModelTotal - Probability) >> aNumMoveBits;
      if (rangeDecoder->Range < kTopValue)
      {
        rangeDecoder->Code = (rangeDecoder->Code << 8) | rangeDecoder->Stream.ReadByte();
        rangeDecoder->Range <<= 8;
      }
      return 0;
    }
    else
    {
      rangeDecoder->Range -= newBound;
      rangeDecoder->Code -= newBound;
      Probability -= (Probability) >> aNumMoveBits;
      if (rangeDecoder->Range < kTopValue)
      {
        rangeDecoder->Code = (rangeDecoder->Code << 8) | rangeDecoder->Stream.ReadByte();
        rangeDecoder->Range <<= 8;
      }
      return 1;
    }
  }
};

}}


#endif
