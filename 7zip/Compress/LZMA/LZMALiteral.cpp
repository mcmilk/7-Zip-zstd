// LZMALiteral.cpp

#include "StdAfx.h"

#include "LZMALiteral.h"

namespace NCompress {
namespace NLZMA {
namespace NLiteral {

void CEncoder2::Init()
{
  for (int i = 0; i < 3; i++)
    for (int j = 1; j < (1 << 8); j++)
      _encoders[i][j].Init();
}

void CEncoder2::Encode(NRangeCoder::CEncoder *rangeEncoder, 
    bool matchMode, BYTE matchByte, BYTE symbol)
{
  UINT32 context = 1;
  bool same = true;
  for (int i = 7; i >= 0; i--)
  {
    UINT32 bit = (symbol >> i) & 1;
    UINT state;
    if (matchMode && same)
    {
      UINT32 matchBit = (matchByte >> i) & 1;
      state = 1 + matchBit;
      same = (matchBit == bit);
    }
    else
      state = 0;
    _encoders[state][context].Encode(rangeEncoder, bit);
    context = (context << 1) | bit;
  }
}

UINT32 CEncoder2::GetPrice(bool matchMode, BYTE matchByte, BYTE symbol) const
{
  UINT32 price = 0;
  UINT32 context = 1;
  int i = 7;
  if (matchMode)
  {
    for (; i >= 0; i--)
    {
      UINT32 matchBit = (matchByte >> i) & 1;
      UINT32 bit = (symbol >> i) & 1;
      price += _encoders[1 + matchBit][context].GetPrice(bit);
      context = (context << 1) | bit;
      if (matchBit != bit)
      {
        i--;
        break;
      }
    }
  }
  for (; i >= 0; i--)
  {
    UINT32 bit = (symbol >> i) & 1;
    price += _encoders[0][context].GetPrice(bit);
    context = (context << 1) | bit;
  }
  return price;
};

}}}
