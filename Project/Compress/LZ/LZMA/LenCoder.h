// LenCoder.h

#pragma once

#ifndef __LENCODER_H
#define __LENCODER_H

#include "BitTreeCoder.h"

namespace NLength {

const kNumPosStatesBitsMax = 4;
const kNumPosStatesMax = (1 << kNumPosStatesBitsMax);

const kNumPosStatesBitsEncodingMax = 4;
const kNumPosStatesEncodingMax = (1 << kNumPosStatesBitsEncodingMax);

const kNumMoveBits = 5;

const kNumLenBits = 3;
const kNumLowSymbols = 1 << kNumLenBits;
const kNumMidBits = 3;
const kNumMidSymbols = 1 << kNumMidBits;

const kNumHighBits = 8;

const kNumSymbolsTotal = kNumLowSymbols + kNumMidSymbols + (1 << kNumHighBits);

class CEncoder
{
  CMyBitEncoder<kNumMoveBits> _choice;
  CBitTreeEncoder<kNumMoveBits, kNumLenBits>  _lowCoder[kNumPosStatesEncodingMax];
  CMyBitEncoder<kNumMoveBits>  _choice2;
  CBitTreeEncoder<kNumMoveBits, kNumMidBits>  _midCoder[kNumPosStatesEncodingMax];
  CBitTreeEncoder<kNumMoveBits, kNumHighBits>  _highCoder;
protected:
  UINT32 _numPosStates;
public:
  void Create(UINT32 numPosStates)
    { _numPosStates = numPosStates; }
  void Init();
  void Encode(CMyRangeEncoder *rangeEncoder, UINT32 symbol, UINT32 posState);

  UINT32 GetPrice(UINT32 symbol, UINT32 posState) const;
};

const kNumSpecSymbols = kNumLowSymbols + kNumMidSymbols;

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
  void Encode(CMyRangeEncoder *rangeEncoder, UINT32 symbol, UINT32 posState)
  {
    CEncoder::Encode(rangeEncoder, symbol, posState);
    if (--_counters[posState] == 0)
      UpdateTable(posState);
  }
};


class CDecoder
{
  CMyBitDecoder<kNumMoveBits> _choice;
  CBitTreeDecoder<kNumMoveBits, kNumLenBits>  _lowCoder[kNumPosStatesMax];
  CMyBitDecoder<kNumMoveBits> _choice2;
  CBitTreeDecoder<kNumMoveBits, kNumMidBits>  _midCoder[kNumPosStatesMax];
  CBitTreeDecoder<kNumMoveBits, kNumHighBits> _highCoder; 
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
  UINT32 Decode(CMyRangeDecoder *rangeDecoder, UINT32 posState)
  {
    if(_choice.Decode(rangeDecoder) == 0)
      return _lowCoder[posState].Decode(rangeDecoder);
    else
    {
      UINT32 symbol = kNumLowSymbols;
      if(_choice2.Decode(rangeDecoder) == 0)
        symbol += _midCoder[posState].Decode(rangeDecoder);
      else
      {
        symbol += kNumMidSymbols;
        symbol += _highCoder.Decode(rangeDecoder);
      }
      return symbol;
    }
  }

};

}


#endif
