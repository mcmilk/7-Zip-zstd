// LZMALen.cpp

#include "StdAfx.h"

#include "LZMALen.h"

namespace NCompress {
namespace NLZMA {
namespace NLength {

void CEncoder::Init()
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

void CEncoder::Encode(NRangeCoder::CEncoder *rangeEncoder, UINT32 symbol, UINT32 posState)
{
  if(symbol < kNumLowSymbols)
  {
    _choice.Encode(rangeEncoder, 0);
    _lowCoder[posState].Encode(rangeEncoder, symbol);
  }
  else
  {
    symbol -= kNumLowSymbols;
    _choice.Encode(rangeEncoder, 1);
    if(symbol < kNumMidSymbols)
    {
      _choice2.Encode(rangeEncoder, 0);
      _midCoder[posState].Encode(rangeEncoder, symbol);
    }
    else
    {
      _choice2.Encode(rangeEncoder, 1);
      _highCoder.Encode(rangeEncoder, symbol - kNumMidSymbols);
    }
  }
}

UINT32 CEncoder::GetPrice(UINT32 symbol, UINT32 posState) const
{
  UINT32 price = 0;
  if(symbol < kNumLowSymbols)
  {
    price += _choice.GetPrice(0);
    price += _lowCoder[posState].GetPrice(symbol);
  }
  else
  {
    symbol -= kNumLowSymbols;
    price += _choice.GetPrice(1);
    if(symbol < kNumMidSymbols)
    {
      price += _choice2.GetPrice(0);
      price += _midCoder[posState].GetPrice(symbol);
    }
    else
    {
      price += _choice2.GetPrice(1);
      price += _highCoder.GetPrice(symbol - kNumMidSymbols);
    }
  }
  return price;
}

}}}

