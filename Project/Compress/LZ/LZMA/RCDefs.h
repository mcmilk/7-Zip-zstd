// RCDefs.h

#pragma once

#ifndef __RCDEFS_H
#define __RCDEFS_H

#include "../../Interface/CompressInterface.h"
#include "Compression/AriBitCoder.h"
#include "AriConst.h"

#define RC_INIT_VAR                            \
  UINT32 range = rangeDecoder->Range;      \
  UINT32 code = rangeDecoder->Code;        

#define RC_FLUSH_VAR                          \
  rangeDecoder->Range = range;            \
  rangeDecoder->Code = code;

#define RC_NORMALIZE                                    \
    if (range < NCompression::NArithmetic::kTopValue)               \
    {                                                              \
      code = (code << 8) | rangeDecoder->Stream.ReadByte();   \
      range <<= 8; }

#define RC_GETBIT2(aNumMoveBits, aProb, aModelIndex, Action0, Action1)                        \
    {UINT32 aNewBound = (range >> NCompression::NArithmetic::kNumBitModelTotalBits) * aProb; \
    if (code < aNewBound)                               \
    {                                                             \
      Action0;                                                    \
      range = aNewBound;                                         \
      aProb += (NCompression::NArithmetic::kBitModelTotal - aProb) >> aNumMoveBits;          \
      aModelIndex <<= 1;                                          \
    }                                                             \
    else                                                          \
    {                                                             \
      Action1;                                                    \
      range -= aNewBound;                                        \
      code -= aNewBound;                                          \
      aProb -= (aProb) >> aNumMoveBits;                           \
      aModelIndex = (aModelIndex << 1) + 1;                       \
    }}                                                             \
    RC_NORMALIZE

#define RC_GETBIT(aNumMoveBits, aProb, aModelIndex) RC_GETBIT2(aNumMoveBits, aProb, aModelIndex, ; , ;)               

#endif