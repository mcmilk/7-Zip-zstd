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
  CMyBitEncoder<kNumMoveBits> m_Choice;
  CBitTreeEncoder<kNumMoveBits, kNumLenBits>  m_LowCoder[kNumPosStatesEncodingMax];
  CMyBitEncoder<kNumMoveBits>  m_Choice2;
  CBitTreeEncoder<kNumMoveBits, kNumMidBits>  m_MidCoder[kNumPosStatesEncodingMax];
  CBitTreeEncoder<kNumMoveBits, kNumHighBits>  m_HighCoder;
protected:
  UINT32 m_NumPosStates;
public:
  void Create(UINT32 aNumPosStates)
    { m_NumPosStates = aNumPosStates; }
  void Init();
  void Encode(CMyRangeEncoder *aRangeEncoder, UINT32 aSymbol, UINT32 aPosState);

  UINT32 GetPrice(UINT32 aSymbol, UINT32 aPosState) const;
};

const kNumSpecSymbols = kNumLowSymbols + kNumMidSymbols;

class CPriceTableEncoder: public CEncoder
{
  UINT32 m_Prices[kNumSymbolsTotal][kNumPosStatesEncodingMax];
  UINT32 m_TableSize;
  UINT32 m_Counters[kNumPosStatesEncodingMax];
public:
  void SetTableSize(UINT32 aTableSize)
    { m_TableSize = aTableSize;  }
  UINT32 GetPrice(UINT32 aSymbol, UINT32 aPosState) const
    { return m_Prices[aSymbol][aPosState]; }
  void UpdateTable(UINT32 aPosState)
  {
    for (UINT32 aLen = 0; aLen < m_TableSize; aLen++)
      m_Prices[aLen][aPosState] = CEncoder::GetPrice(aLen , aPosState);
    m_Counters[aPosState] = m_TableSize;
  }
  void UpdateTables()
  {
    for (UINT32 aPosState = 0; aPosState < m_NumPosStates; aPosState++)
      UpdateTable(aPosState);
  }
  void Encode(CMyRangeEncoder *aRangeEncoder, UINT32 aSymbol, UINT32 aPosState)
  {
    CEncoder::Encode(aRangeEncoder, aSymbol, aPosState);
    if (--m_Counters[aPosState] == 0)
      UpdateTable(aPosState);
  }
};


class CDecoder
{
  CMyBitDecoder<kNumMoveBits> m_Choice;
  CBitTreeDecoder<kNumMoveBits, kNumLenBits>  m_LowCoder[kNumPosStatesMax];
  CMyBitDecoder<kNumMoveBits> m_Choice2;
  CBitTreeDecoder<kNumMoveBits, kNumMidBits>  m_MidCoder[kNumPosStatesMax];
  CBitTreeDecoder<kNumMoveBits, kNumHighBits> m_HighCoder; 
  UINT32 m_NumPosStates;
public:
  void Create(UINT32 aNumPosStates)
    { m_NumPosStates = aNumPosStates; }
  void Init()
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
  UINT32 Decode(CMyRangeDecoder *aRangeDecoder, UINT32 aPosState)
  {
    if(m_Choice.Decode(aRangeDecoder) == 0)
      return m_LowCoder[aPosState].Decode(aRangeDecoder);
    else
    {
      UINT32 aSymbol = kNumLowSymbols;
      if(m_Choice2.Decode(aRangeDecoder) == 0)
        aSymbol += m_MidCoder[aPosState].Decode(aRangeDecoder);
      else
      {
        aSymbol += kNumMidSymbols;
        aSymbol += m_HighCoder.Decode(aRangeDecoder);
      }
      return aSymbol;
    }
  }

};

}


#endif
