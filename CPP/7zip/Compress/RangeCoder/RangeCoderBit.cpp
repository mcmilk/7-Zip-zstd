// Compress/RangeCoder/RangeCoderBit.cpp

#include "StdAfx.h"

#include "RangeCoderBit.h"

namespace NCompress {
namespace NRangeCoder {

UInt32 ProbPrices[kBitModelTotal >> kNumMoveReducingBits];

struct CPriceTables { CPriceTables()   
{
  for (UInt32 i = (1 << kNumMoveReducingBits) / 2; i < kBitModelTotal; i += (1 << kNumMoveReducingBits))
  {
    const int kCyclesBits = kNumBitPriceShiftBits;
    UInt32 w = i;
    UInt32 bitCount = 0;
    for (int j = 0; j < kCyclesBits; j++)
    {
      w = w * w;
      bitCount <<= 1;
      while (w >= ((UInt32)1 << 16))
      {
        w >>= 1;
        bitCount++;
      }
    }
    ProbPrices[i >> kNumMoveReducingBits] = ((kNumBitModelTotalBits << kCyclesBits) - 15 - bitCount);
  }
}
};

static CPriceTables g_PriceTables;

}}
