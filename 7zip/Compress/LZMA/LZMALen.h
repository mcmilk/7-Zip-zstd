// LenCoder.h

// #pragma once

#ifndef __LENCODER_H
#define __LENCODER_H

#include "../RangeCoder/RangeCoderBitTree.h"

namespace NCompress {
namespace NLZMA {
namespace NLength {

const int kNumMoveBits = 5;

const int kNumPosStatesBitsMax = 4;
const UINT32 kNumPosStatesMax = (1 << kNumPosStatesBitsMax);

const int kNumPosStatesBitsEncodingMax = 4;
const UINT32 kNumPosStatesEncodingMax = (1 << kNumPosStatesBitsEncodingMax);

const int kNumLenBits = 3;
const UINT32 kNumLowSymbols = 1 << kNumLenBits;
const int kNumMidBits = 3;
const UINT32 kNumMidSymbols = 1 << kNumMidBits;

const int kNumHighBits = 8;

const UINT32 kNumSymbolsTotal = kNumLowSymbols + kNumMidSymbols + (1 << kNumHighBits);

class CEncoder
{
  NRangeCoder::CBitEncoder<kNumMoveBits> _choice;
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumLenBits>  _lowCoder[kNumPosStatesEncodingMax];
  NRangeCoder::CBitEncoder<kNumMoveBits>  _choice2;
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumMidBits>  _midCoder[kNumPosStatesEncodingMax];
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumHighBits>  _highCoder;
protected:
  UINT32 _numPosStates;
public:
  void Create(UINT32 numPosStates)
    { _numPosStates = numPosStates; }
  void Init();
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UINT32 symbol, UINT32 posState);
  UINT32 GetPrice(UINT32 symbol, UINT32 posState) const;
};

const UINT32 kNumSpecSymbols = kNumLowSymbols + kNumMidSymbols;

class CPriceTableEncoder: public CEncoder
{
  UINT32 _prices[kNumSymbolsTotal][kNumPosStatesEncodingMax];
  UINT32 _tableSize;
  UINT32 _counters[kNumPosStatesEncodingMax];
public:
  void SetTableSize(UINT32 tableSize)
    { _tableSize = tableSize;  }
  UINT32 GetPrice(UINT32 symbol, UINT32 posState) const
    { return _prices[symbol][posState]; }
  void UpdateTable(UINT32 posState)
  {
    for (UINT32 len = 0; len < _tableSize; len++)
      _prices[len][posState] = CEncoder::GetPrice(len , posState);
    _counters[posState] = _tableSize;
  }
  void UpdateTables()
  {
    for (UINT32 posState = 0; posState < _numPosStates; posState++)
      UpdateTable(posState);
  }
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UINT32 symbol, UINT32 posState)
  {
    CEncoder::Encode(rangeEncoder, symbol, posState);
    if (--_counters[posState] == 0)
      UpdateTable(posState);
  }
};


class CDecoder
{
  NRangeCoder::CBitDecoder<kNumMoveBits> _choice;
  NRangeCoder::CBitTreeDecoder<kNumMoveBits, kNumLenBits>  _lowCoder[kNumPosStatesMax];
  NRangeCoder::CBitDecoder<kNumMoveBits> _choice2;
  NRangeCoder::CBitTreeDecoder<kNumMoveBits, kNumMidBits>  _midCoder[kNumPosStatesMax];
  NRangeCoder::CBitTreeDecoder<kNumMoveBits, kNumHighBits> _highCoder; 
  UINT32 _numPosStates;
public:
  void Create(UINT32 numPosStates)
    { _numPosStates = numPosStates; }
  void Init()
  {
    _choice.Init();
    for (UINT32 posState = 0; posState < _numPosStates; posState++)
    {
      _lowCoder[posState].Init();
      _midCoder[posState].Init();
    }
    _choice2.Init();
    _highCoder.Init();
  }
  UINT32 Decode(NRangeCoder::CDecoder *rangeDecoder, UINT32 posState)
  {
    if(_choice.Decode(rangeDecoder) == 0)
      return _lowCoder[posState].Decode(rangeDecoder);
    if(_choice2.Decode(rangeDecoder) == 0)
      return kNumLowSymbols + _midCoder[posState].Decode(rangeDecoder);
    return kNumLowSymbols + kNumMidSymbols + _highCoder.Decode(rangeDecoder);
  }
};

}}}

#endif
