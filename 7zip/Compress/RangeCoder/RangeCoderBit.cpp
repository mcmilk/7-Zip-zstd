// Compress/RangeCoder/RangeCoderBit.cpp

#include "StdAfx.h"

#include "RangeCoderBit.h"

namespace NCompress {
namespace NRangeCoder {

CPriceTables::CPriceTables()
{
  /*
  // simplest: bad solution
  for(UINT32 i = 1; i < (kBitModelTotal >> kNumMoveReducingBits) - 1; i++)
    StatePrices[i] = kBitPrice;
  */
  
  /*
  const double kDummyMultMid = (1.0 / kBitPrice) / 2;
  const double kDummyMultMid = 0;
  // float solution
  double ln2 = log(double(2));
  double lnAll = log(double(kBitModelTotal >> kNumMoveReducingBits));
  for(UINT32 i = 1; i < (kBitModelTotal >> kNumMoveReducingBits) - 1; i++)
    StatePrices[i] = UINT32((fabs(lnAll - log(double(i))) / ln2 + kDummyMultMid) * kBitPrice);
  */
  
  /*
  // experimental, slow, solution:
  for(UINT32 i = 1; i < (kBitModelTotal >> kNumMoveReducingBits) - 1; i++)
  {
    const int kCyclesBits = 5;
    const UINT32 kCycles = (1 << kCyclesBits);

    UINT32 range = UINT32(-1);
    UINT32 bitCount = 0;
    for (UINT32 j = 0; j < kCycles; j++)
    {
      range >>= (kNumBitModelTotalBits - kNumMoveReducingBits);
      range *= i;
      while(range < (1 << 31))
      {
        range <<= 1;
        bitCount++;
      }
    }
    bitCount <<= kNumBitPriceShiftBits;
    range -= (1 << 31);
    for (int k = kNumBitPriceShiftBits - 1; k >= 0; k--)
    {
      range <<= 1;
      if (range > (1 << 31))
      {
        bitCount += (1 << k);
        range -= (1 << 31);
      }
    }
    StatePrices[i] = (bitCount 
      // + (1 << (kCyclesBits - 1))
      ) >> kCyclesBits;
  }
  */

  const int kNumBits = (kNumBitModelTotalBits - kNumMoveReducingBits);
  for(int i = kNumBits - 1; i >= 0; i--)
  {
    UINT32 start = 1 << (kNumBits - i - 1);
    UINT32 end = 1 << (kNumBits - i);
    for (UINT32 j = start; j < end; j++)
      StatePrices[j] = (i << kNumBitPriceShiftBits) + 
          (((end - j) << kNumBitPriceShiftBits) >> (kNumBits - i - 1));
  }
}

CPriceTables g_PriceTables;

}}
