// Compress/RangeCoder/RangeCoderBit.h

// #pragma once

#ifndef __COMPRESS_RANGECODER_BIT_TREE_H
#define __COMPRESS_RANGECODER_BIT_TREE_H

#include "RangeCoder.h"

namespace NCompress {
namespace NRangeCoder {

const int kNumBitModelTotalBits  = 11;
const UINT32 kBitModelTotal = (1 << kNumBitModelTotalBits);

const int kNumMoveReducingBits = 2;

const int kNumBitPriceShiftBits = 6;
const UINT32 kBitPrice = 1 << kNumBitPriceShiftBits;

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
  void Encode(CEncoder *encoder, UINT32 symbol)
  {
    encoder->EncodeBit(Probability, kNumBitModelTotalBits, symbol);
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
  UINT32 Decode(CDecoder *decoder)
  {
    UINT32 newBound = (decoder->Range >> kNumBitModelTotalBits) * Probability;
    if (decoder->Code < newBound)
    {
      decoder->Range = newBound;
      Probability += (kBitModelTotal - Probability) >> aNumMoveBits;
      if (decoder->Range < kTopValue)
      {
        decoder->Code = (decoder->Code << 8) | decoder->Stream.ReadByte();
        decoder->Range <<= 8;
      }
      return 0;
    }
    else
    {
      decoder->Range -= newBound;
      decoder->Code -= newBound;
      Probability -= (Probability) >> aNumMoveBits;
      if (decoder->Range < kTopValue)
      {
        decoder->Code = (decoder->Code << 8) | decoder->Stream.ReadByte();
        decoder->Range <<= 8;
      }
      return 1;
    }
  }
};

}}


#endif
