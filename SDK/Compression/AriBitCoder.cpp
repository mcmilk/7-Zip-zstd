// Compression/AriBitCoder.cpp

#include "StdAfx.h"

#include <math.h>

#include "AriBitCoder.h"
#include "Common/Defs.h"
#include "AriPrice.h"

namespace NCompression {
namespace NArithmetic {

// static const double kDummyMultMid = (1.0 / kBitPrice) / 2;
static const double kDummyMultMid = (1.0 / kBitPrice) / 2;
// static const double kDummyMultMid = 0;

CPriceTables::CPriceTables()
{
  double aLn2 = log(2);
  double aLnAll = log(kBitModelTotal >> kNumMoveReducingBits);
  for(UINT32 i = 1; i < (kBitModelTotal >> kNumMoveReducingBits) - 1; i++)
    m_StatePrices[i] = UINT32((fabs(aLnAll - log(i)) / aLn2 + kDummyMultMid) * kBitPrice);
}

CPriceTables g_PriceTables;

}}
