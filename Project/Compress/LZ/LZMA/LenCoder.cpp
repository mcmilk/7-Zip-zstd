// LenCoder.cpp

#include "StdAfx.h"

#include "LenCoder.h"

using namespace NCompression;
using namespace NArithmetic;

namespace NLength {

void CEncoder::Init()
{
  m_Choice.Init();
  for (UINT32 aPosState = 0; aPosState < m_NumPosStates; aPosState++)
  {
    m_LowCoder[aPosState].Init();
    m_MidCoder[aPosState].Init();
  }
  m_Choice2.Init();
  m_HighCoder.Init();
}

void CEncoder::Encode(CMyRangeEncoder *aRangeEncoder, UINT32 aSymbol, UINT32 aPosState)
{
  if(aSymbol < kNumLowSymbols)
  {
    m_Choice.Encode(aRangeEncoder, 0);
    m_LowCoder[aPosState].Encode(aRangeEncoder, aSymbol);
  }
  else
  {
    aSymbol -= kNumLowSymbols;
    m_Choice.Encode(aRangeEncoder, 1);
    if(aSymbol < kNumMidSymbols)
    {
      m_Choice2.Encode(aRangeEncoder, 0);
      m_MidCoder[aPosState].Encode(aRangeEncoder, aSymbol);
    }
    else
    {
      aSymbol -= kNumMidSymbols;
      m_Choice2.Encode(aRangeEncoder, 1);
      m_HighCoder.Encode(aRangeEncoder, aSymbol);
    }
  }
}

UINT32 CEncoder::GetPrice(UINT32 aSymbol, UINT32 aPosState) const
{
  UINT32 aPrice = 0;
  if(aSymbol < kNumLowSymbols)
  {
    aPrice += m_Choice.GetPrice(0);
    aPrice += m_LowCoder[aPosState].GetPrice(aSymbol);
  }
  else
  {
    aSymbol -= kNumLowSymbols;
    aPrice += m_Choice.GetPrice(1);
    if(aSymbol < kNumMidSymbols)
    {
      aPrice += m_Choice2.GetPrice(0);
      aPrice += m_MidCoder[aPosState].GetPrice(aSymbol);
    }
    else
    {
      aSymbol -= kNumMidSymbols;
      aPrice += m_Choice2.GetPrice(1);
      aPrice += m_HighCoder.GetPrice(aSymbol);
    }
  }
  return aPrice;
}

}

