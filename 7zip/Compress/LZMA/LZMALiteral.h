// LiteralCoder.h

// #pragma once

#ifndef __LITERALCODER_H
#define __LITERALCODER_H

#include "../RangeCoder/RangeCoderBit.h"
#include "../RangeCoder/RangeCoderOpt.h"

namespace NCompress {
namespace NLZMA {
namespace NLiteral {

const int kNumMoveBits = 5;

class CEncoder2
{
  NRangeCoder::CBitEncoder<kNumMoveBits> _encoders[3][1 << 8];
public:
  void Init();
  void Encode(NRangeCoder::CEncoder *rangeEncoder, bool matchMode, BYTE matchByte, BYTE symbol);
  UINT32 GetPrice(bool matchMode, BYTE matchByte, BYTE symbol) const;
};

class CDecoder2
{
  NRangeCoder::CBitDecoder<kNumMoveBits> _decoders[3][1 << 8];
public:
  void Init()
  {
    for (int i = 0; i < 3; i++)
      for (int j = 1; j < (1 << 8); j++)
        _decoders[i][j].Init();
  }

  BYTE DecodeNormal(NRangeCoder::CDecoder *rangeDecoder)
  {
    UINT32 symbol = 1;
    RC_INIT_VAR
    do
    {
      // symbol = (symbol << 1) | _decoders[0][symbol].Decode(rangeDecoder);
      RC_GETBIT(kNumMoveBits, _decoders[0][symbol].Probability, symbol)
    }
    while (symbol < 0x100);
    RC_FLUSH_VAR
    return symbol;
  }

  BYTE DecodeWithMatchByte(NRangeCoder::CDecoder *rangeDecoder, BYTE matchByte)
  {
    UINT32 symbol = 1;
    RC_INIT_VAR
    do
    {
      UINT32 matchBit = (matchByte >> 7) & 1;
      matchByte <<= 1;
      // UINT32 bit = _decoders[1 + matchBit][symbol].Decode(rangeDecoder);
      // symbol = (symbol << 1) | bit;
      UINT32 bit;
      RC_GETBIT2(kNumMoveBits, _decoders[1 + matchBit][symbol].Probability, symbol, 
          bit = 0, bit = 1)
      if (matchBit != bit)
      {
        while (symbol < 0x100)
        {
          // symbol = (symbol << 1) | _decoders[0][symbol].Decode(rangeDecoder);
          RC_GETBIT(kNumMoveBits, _decoders[0][symbol].Probability, symbol)
        }
        break;
      }
    }
    while (symbol < 0x100);
    RC_FLUSH_VAR
    return symbol;
  }
};

/*
const UINT32 kNumPrevByteBits = 1;
const UINT32 kNumPrevByteStates =  (1 << kNumPrevByteBits);

inline UINT32 GetLiteralState(BYTE prevByte)
  { return (prevByte >> (8 - kNumPrevByteBits)); }
*/

class CEncoder
{
  CEncoder2 *_coders;
  UINT32 _numPrevBits;
  UINT32 _numPosBits;
  UINT32 _posMask;
public:
  CEncoder(): _coders(0) {}
  ~CEncoder()  { Free(); }
  void Free()
  { 
    delete []_coders;
    _coders = 0;
  }
  void Create(UINT32 numPosBits, UINT32 numPrevBits)
  {
    Free();
    _numPosBits = numPosBits;
    _posMask = (1 << numPosBits) - 1;
    _numPrevBits = numPrevBits;
    UINT32 numStates = 1 << (_numPrevBits + _numPosBits);
    _coders = new CEncoder2[numStates];
  }
  void Init()
  {
    UINT32 numStates = 1 << (_numPrevBits + _numPosBits);
    for (UINT32 i = 0; i < numStates; i++)
      _coders[i].Init();
  }
  UINT32 GetState(UINT32 pos, BYTE prevByte) const
    { return ((pos & _posMask) << _numPrevBits) + (prevByte >> (8 - _numPrevBits)); }
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UINT32 pos, BYTE prevByte, 
      bool matchMode, BYTE matchByte, BYTE symbol)
    { _coders[GetState(pos, prevByte)].Encode(rangeEncoder, matchMode, 
          matchByte, symbol); }
  UINT32 GetPrice(UINT32 pos, BYTE prevByte, bool matchMode, BYTE matchByte, BYTE symbol) const
    { return _coders[GetState(pos, prevByte)].GetPrice(matchMode, matchByte, symbol); }
};

class CDecoder
{
  CDecoder2 *_coders;
  UINT32 _numPrevBits;
  UINT32 _numPosBits;
  UINT32 _posMask;
public:
  CDecoder(): _coders(0) {}
  ~CDecoder()  { Free(); }
  void Free()
  { 
    delete []_coders;
    _coders = 0;
  }
  void Create(UINT32 numPosBits, UINT32 numPrevBits)
  {
    Free();
    _numPosBits = numPosBits;
    _posMask = (1 << numPosBits) - 1;
    _numPrevBits = numPrevBits;
    UINT32 numStates = 1 << (_numPrevBits + _numPosBits);
    _coders = new CDecoder2[numStates];
  }
  void Init()
  {
    UINT32 numStates = 1 << (_numPrevBits + _numPosBits);
    for (UINT32 i = 0; i < numStates; i++)
      _coders[i].Init();
  }
  UINT32 GetState(UINT32 pos, BYTE prevByte) const
    { return ((pos & _posMask) << _numPrevBits) + (prevByte >> (8 - _numPrevBits)); }
  BYTE DecodeNormal(NRangeCoder::CDecoder *rangeDecoder, UINT32 pos, BYTE prevByte)
    { return _coders[GetState(pos, prevByte)].DecodeNormal(rangeDecoder); }
  BYTE DecodeWithMatchByte(NRangeCoder::CDecoder *rangeDecoder, UINT32 pos, BYTE prevByte, BYTE matchByte)
    { return _coders[GetState(pos, prevByte)].DecodeWithMatchByte(rangeDecoder, matchByte); }
};

}}}

#endif
