// Compress/RangeCoder/RangeCoderOpt.h

// #pragma once

#ifndef __COMPRESS_RANGECODER_OPT_H
#define __COMPRESS_RANGECODER_OPT_H

#define RC_INIT_VAR                            \
  UINT32 range = rangeDecoder->Range;      \
  UINT32 code = rangeDecoder->Code;        

#define RC_FLUSH_VAR                          \
  rangeDecoder->Range = range;            \
  rangeDecoder->Code = code;

#define RC_NORMALIZE                                    \
    if (range < NCompress::NRangeCoder::kTopValue)               \
    {                                                              \
      code = (code << 8) | rangeDecoder->Stream.ReadByte();   \
      range <<= 8; }

#define RC_GETBIT2(numMoveBits, prob, modelIndex, Action0, Action1)                        \
    {UINT32 newBound = (range >> NCompress::NRangeCoder::kNumBitModelTotalBits) * prob; \
    if (code < newBound)                               \
    {                                                             \
      Action0;                                                    \
      range = newBound;                                         \
      prob += (NCompress::NRangeCoder::kBitModelTotal - prob) >> numMoveBits;          \
      modelIndex <<= 1;                                          \
    }                                                             \
    else                                                          \
    {                                                             \
      Action1;                                                    \
      range -= newBound;                                        \
      code -= newBound;                                          \
      prob -= (prob) >> numMoveBits;                           \
      modelIndex = (modelIndex << 1) + 1;                       \
    }}                                                             \
    RC_NORMALIZE

#define RC_GETBIT(numMoveBits, prob, modelIndex) RC_GETBIT2(numMoveBits, prob, modelIndex, ; , ;)               

#endif
