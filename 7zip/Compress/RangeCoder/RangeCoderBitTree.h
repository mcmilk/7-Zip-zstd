// Compress/RangeCoder/RangeCoderBitTree.h

// #pragma once

#ifndef __COMPRESS_RANGECODER_BIT_H
#define __COMPRESS_RANGECODER_BIT_H

#include "RangeCoderBit.h"
#include "RangeCoderOpt.h"

namespace NCompress {
namespace NRangeCoder {

/*
template <int numMoveBits> class CMyBitEncoder: 
    public NCompression::NArithmetic::CBitEncoder<numMoveBits> {};
template <int numMoveBits> class CMyBitDecoder: 
    public NCompression::NArithmetic::CBitDecoder<numMoveBits> {};
*/

//////////////////////////
// CBitTreeEncoder

template <int numMoveBits, UINT32 NumBitLevels>
class CBitTreeEncoder
{
  CBitEncoder<numMoveBits> Models[1 << NumBitLevels];
public:
  void Init()
  {
    for(UINT32 i = 1; i < (1 << NumBitLevels); i++)
      Models[i].Init();
  }
  void Encode(CEncoder *rangeEncoder, UINT32 symbol)
  {
    UINT32 modelIndex = 1;
    for (UINT32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UINT32 bit = (symbol >> bitIndex ) & 1;
      Models[modelIndex].Encode(rangeEncoder, bit);
      modelIndex = (modelIndex << 1) | bit;
    }
  };
  UINT32 GetPrice(UINT32 symbol) const
  {
    UINT32 price = 0;
    UINT32 modelIndex = 1;
    for (UINT32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UINT32 bit = (symbol >> bitIndex ) & 1;
      price += Models[modelIndex].GetPrice(bit);
      modelIndex = (modelIndex << 1) + bit;
    }
    return price;
  }
};

//////////////////////////
// CBitTreeDecoder

template <int numMoveBits, UINT32 NumBitLevels>
class CBitTreeDecoder
{
  CBitDecoder<numMoveBits> Models[1 << NumBitLevels];
public:
  void Init()
  {
    for(UINT32 i = 1; i < (1 << NumBitLevels); i++)
      Models[i].Init();
  }
  UINT32 Decode(CDecoder *rangeDecoder)
  {
    UINT32 modelIndex = 1;
    RC_INIT_VAR
    for(UINT32 bitIndex = NumBitLevels; bitIndex > 0; bitIndex--)
    {
      // modelIndex = (modelIndex << 1) + Models[modelIndex].Decode(rangeDecoder);
      RC_GETBIT(numMoveBits, Models[modelIndex].Probability, modelIndex)
    }
    RC_FLUSH_VAR
    return modelIndex - (1 << NumBitLevels);
  };
};

////////////////////////////////
// CReverseBitTreeEncoder

template <int numMoveBits>
class CReverseBitTreeEncoder2
{
  CBitEncoder<numMoveBits> *Models;
  UINT32 NumBitLevels;
public:
  CReverseBitTreeEncoder2(): Models(0) { }
  ~CReverseBitTreeEncoder2() { delete []Models; }
  void Create(UINT32 numBitLevels)
  {
    NumBitLevels = numBitLevels;
    Models = new CBitEncoder<numMoveBits>[1 << numBitLevels];
    // return (Models != 0);
  }
  void Init()
  {
    UINT32 numModels = 1 << NumBitLevels;
    for(UINT32 i = 1; i < numModels; i++)
      Models[i].Init();
  }
  void Encode(CEncoder *rangeEncoder, UINT32 symbol)
  {
    UINT32 modelIndex = 1;
    for (UINT32 i = 0; i < NumBitLevels; i++)
    {
      UINT32 bit = symbol & 1;
      Models[modelIndex].Encode(rangeEncoder, bit);
      modelIndex = (modelIndex << 1) | bit;
      symbol >>= 1;
    }
  }
  UINT32 GetPrice(UINT32 symbol) const
  {
    UINT32 price = 0;
    UINT32 modelIndex = 1;
    for (UINT32 i = NumBitLevels; i > 0; i--)
    {
      UINT32 bit = symbol & 1;
      symbol >>= 1;
      price += Models[modelIndex].GetPrice(bit);
      modelIndex = (modelIndex << 1) | bit;
    }
    return price;
  }
};

/*
template <int numMoveBits, int numBitLevels>
class CReverseBitTreeEncoder: public CReverseBitTreeEncoder2<numMoveBits>
{
public:
  CReverseBitTreeEncoder() 
    { Create(numBitLevels); }
};
*/
////////////////////////////////
// CReverseBitTreeDecoder

template <int numMoveBits>
class CReverseBitTreeDecoder2
{
  CBitDecoder<numMoveBits> *Models;
  UINT32 NumBitLevels;
public:
  CReverseBitTreeDecoder2(): Models(0) { }
  ~CReverseBitTreeDecoder2() { delete []Models; }
  void Create(UINT32 numBitLevels)
  {
    NumBitLevels = numBitLevels;
    Models = new CBitDecoder<numMoveBits>[1 << numBitLevels];
    // return (Models != 0);
  }
  void Init()
  {
    UINT32 numModels = 1 << NumBitLevels;
    for(UINT32 i = 1; i < numModels; i++)
      Models[i].Init();
  }
  UINT32 Decode(CDecoder *rangeDecoder)
  {
    UINT32 modelIndex = 1;
    UINT32 symbol = 0;
    RC_INIT_VAR
    for(UINT32 bitIndex = 0; bitIndex < NumBitLevels; bitIndex++)
    {
      // UINT32 bit = Models[modelIndex].Decode(rangeDecoder);
      // modelIndex <<= 1;
      // modelIndex += bit;
      // symbol |= (bit << bitIndex);
      RC_GETBIT2(numMoveBits, Models[modelIndex].Probability, modelIndex, ; , symbol |= (1 << bitIndex))
    }
    RC_FLUSH_VAR
    return symbol;
  };
};
////////////////////////////
// CReverseBitTreeDecoder2

template <int numMoveBits, UINT32 NumBitLevels>
class CReverseBitTreeDecoder
{
  CBitDecoder<numMoveBits> Models[1 << NumBitLevels];
public:
  void Init()
  {
    for(UINT32 i = 1; i < (1 << NumBitLevels); i++)
      Models[i].Init();
  }
  UINT32 Decode(CDecoder *rangeDecoder)
  {
    UINT32 modelIndex = 1;
    UINT32 symbol = 0;
    RC_INIT_VAR
    for(UINT32 bitIndex = 0; bitIndex < NumBitLevels; bitIndex++)
    {
      // UINT32 bit = Models[modelIndex].Decode(rangeDecoder);
      // modelIndex <<= 1;
      // modelIndex += bit;
      // symbol |= (bit << bitIndex);
      RC_GETBIT2(numMoveBits, Models[modelIndex].Probability, modelIndex, ; , symbol |= (1 << bitIndex))
    }
    RC_FLUSH_VAR
    return symbol;
  }
};

/*
//////////////////////////
// CBitTreeEncoder2

template <int numMoveBits>
class CBitTreeEncoder2
{
  NCompression::NArithmetic::CBitEncoder<numMoveBits> *Models;
  UINT32 NumBitLevels;
public:
  bool Create(UINT32 numBitLevels)
  { 
    NumBitLevels = numBitLevels;
    Models = new NCompression::NArithmetic::CBitEncoder<numMoveBits>[1 << numBitLevels];
    return (Models != 0);
  }
  void Init()
  {
    UINT32 numModels = 1 << NumBitLevels;
    for(UINT32 i = 1; i < numModels; i++)
      Models[i].Init();
  }
  void Encode(CMyRangeEncoder *rangeEncoder, UINT32 symbol)
  {
    UINT32 modelIndex = 1;
    for (UINT32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UINT32 bit = (symbol >> bitIndex ) & 1;
      Models[modelIndex].Encode(rangeEncoder, bit);
      modelIndex = (modelIndex << 1) | bit;
    }
  }
  UINT32 GetPrice(UINT32 symbol) const
  {
    UINT32 price = 0;
    UINT32 modelIndex = 1;
    for (UINT32 bitIndex = NumBitLevels; bitIndex > 0 ;)
    {
      bitIndex--;
      UINT32 bit = (symbol >> bitIndex ) & 1;
      price += Models[modelIndex].GetPrice(bit);
      modelIndex = (modelIndex << 1) + bit;
    }
    return price;
  }
};


//////////////////////////
// CBitTreeDecoder2

template <int numMoveBits>
class CBitTreeDecoder2
{
  NCompression::NArithmetic::CBitDecoder<numMoveBits> *Models;
  UINT32 NumBitLevels;
public:
  bool Create(UINT32 numBitLevels)
  { 
    NumBitLevels = numBitLevels;
    Models = new NCompression::NArithmetic::CBitDecoder<numMoveBits>[1 << numBitLevels];
    return (Models != 0);
  }
  void Init()
  {
    UINT32 numModels = 1 << NumBitLevels;
    for(UINT32 i = 1; i < numModels; i++)
      Models[i].Init();
  }
  UINT32 Decode(CMyRangeDecoder *rangeDecoder)
  {
    UINT32 modelIndex = 1;
    RC_INIT_VAR
    for(UINT32 bitIndex = NumBitLevels; bitIndex > 0; bitIndex--)
    {
      // modelIndex = (modelIndex << 1) + Models[modelIndex].Decode(rangeDecoder);
      RC_GETBIT(numMoveBits, Models[modelIndex].Probability, modelIndex)
    }
    RC_FLUSH_VAR
    return modelIndex - (1 << NumBitLevels);
  }
};
*/

}}

#endif
