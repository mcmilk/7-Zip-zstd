// LiteralCoder.cpp

#include "StdAfx.h"

#include "LiteralCoder.h"

using namespace NCompression;
using namespace NArithmetic;

namespace NLiteral {

void CEncoder2::Init()
{
  for (int i = 0; i < 3; i++)
    for (int j = 1; j < (1 << 8); j++)
      m_Encoders[i][j].Init();
}

void CEncoder2::Encode(CMyRangeEncoder *aRangeEncoder, 
    bool aMatchMode, BYTE aMatchByte, BYTE aSymbol)
{
  UINT32 aContext = 1;
  bool aSame = true;
  for (int i = 7; i >= 0; i--)
  {
    UINT32 aBit = (aSymbol >> i) & 1;
    UINT aState;
    if (aMatchMode && aSame)
    {
      UINT32 aMatchBit = (aMatchByte >> i) & 1;
      aState = 1 + aMatchBit;
      aSame = (aMatchBit == aBit);
    }
    else
      aState = 0;
    m_Encoders[aState][aContext].Encode(aRangeEncoder, aBit);
    aContext = (aContext << 1) | aBit;
  }
}

UINT32 CEncoder2::GetPrice(bool aMatchMode, BYTE aMatchByte, BYTE aSymbol) const
{
  UINT32 aPrice = 0;
  UINT32 aContext = 1;
  int i = 7;
  if (aMatchMode)
  {
    for (; i >= 0; i--)
    {
      UINT32 aMatchBit = (aMatchByte >> i) & 1;
      UINT32 aBit = (aSymbol >> i) & 1;
      aPrice += m_Encoders[1 + aMatchBit][aContext].GetPrice(aBit);
      aContext = (aContext << 1) | aBit;
      if (aMatchBit != aBit)
      {
        i--;
        break;
      }
    }
  }
  for (; i >= 0; i--)
  {
    UINT32 aBit = (aSymbol >> i) & 1;
    aPrice += m_Encoders[0][aContext].GetPrice(aBit);
    aContext = (aContext << 1) | aBit;
  }
  return aPrice;
};

}
