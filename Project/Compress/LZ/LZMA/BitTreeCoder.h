// BitTreeCoder.h

#pragma once

#ifndef __BITTREECODER_H
#define __BITTREECODER_H

#include "../../Interface/CompressInterface.h"
#include "Compression/AriBitCoder.h"
#include "RCDefs.h"


//////////////////////////
// CBitTreeEncoder

template <int aNumMoveBits, UINT32 m_NumBitLevels>
class CBitTreeEncoder
{
  CMyBitEncoder<aNumMoveBits> m_Models[1 << m_NumBitLevels];
public:
  void Init()
  {
    for(UINT32 i = 1; i < (1 << m_NumBitLevels); i++)
      m_Models[i].Init();
  }
  void Encode(CMyRangeEncoder *aRangeEncoder, UINT32 aSymbol)
  {
    UINT32 aModelIndex = 1;
    for (UINT32 aBitIndex = m_NumBitLevels; aBitIndex > 0 ;)
    {
      aBitIndex--;
      UINT32 aBit = (aSymbol >> aBitIndex ) & 1;
      m_Models[aModelIndex].Encode(aRangeEncoder, aBit);
      aModelIndex = (aModelIndex << 1) | aBit;
    }
  };
  UINT32 GetPrice(UINT32 aSymbol) const
  {
    UINT32 aPrice = 0;
    UINT32 aModelIndex = 1;
    for (UINT32 aBitIndex = m_NumBitLevels; aBitIndex > 0 ;)
    {
      aBitIndex--;
      UINT32 aBit = (aSymbol >> aBitIndex ) & 1;
      aPrice += m_Models[aModelIndex].GetPrice(aBit);
      aModelIndex = (aModelIndex << 1) + aBit;
    }
    return aPrice;
  }
};

//////////////////////////
// CBitTreeDecoder

template <int aNumMoveBits, UINT32 m_NumBitLevels>
class CBitTreeDecoder
{
  CMyBitDecoder<aNumMoveBits> m_Models[1 << m_NumBitLevels];
public:
  void Init()
  {
    for(UINT32 i = 1; i < (1 << m_NumBitLevels); i++)
      m_Models[i].Init();
  }
  UINT32 Decode(CMyRangeDecoder *aRangeDecoder)
  {
    UINT32 aModelIndex = 1;
    RC_INIT_VAR
    for(UINT32 aBitIndex = m_NumBitLevels; aBitIndex > 0; aBitIndex--)
    {
      // aModelIndex = (aModelIndex << 1) + m_Models[aModelIndex].Decode(aRangeDecoder);
      RC_GETBIT(aNumMoveBits, m_Models[aModelIndex].m_Probability, aModelIndex)
    }
    RC_FLUSH_VAR
    return aModelIndex - (1 << m_NumBitLevels);
  };
};



////////////////////////////////
// CReverseBitTreeEncoder

template <int aNumMoveBits>
class CReverseBitTreeEncoder2
{
  CMyBitEncoder<aNumMoveBits> *m_Models;
  UINT32 m_NumBitLevels;
public:
  CReverseBitTreeEncoder2(): m_Models(0) { }
  ~CReverseBitTreeEncoder2() { delete []m_Models; }
  bool Create(UINT32 aNumBitLevels)
  {
    m_NumBitLevels = aNumBitLevels;
    m_Models = new CMyBitEncoder<aNumMoveBits>[1 << aNumBitLevels];
    return (m_Models != 0);
  }
  void Init()
  {
    UINT32 aNumModels = 1 << m_NumBitLevels;
    for(UINT32 i = 1; i < aNumModels; i++)
      m_Models[i].Init();
  }
  void Encode(CMyRangeEncoder *aRangeEncoder, UINT32 aSymbol)
  {
    UINT32 aModelIndex = 1;
    for (UINT32 i = 0; i < m_NumBitLevels; i++)
    {
      UINT32 aBit = aSymbol & 1;
      m_Models[aModelIndex].Encode(aRangeEncoder, aBit);
      aModelIndex = (aModelIndex << 1) | aBit;
      aSymbol >>= 1;
    }
  }
  UINT32 GetPrice(UINT32 aSymbol) const
  {
    UINT32 aPrice = 0;
    UINT32 aModelIndex = 1;
    for (UINT32 i = m_NumBitLevels; i > 0; i--)
    {
      UINT32 aBit = aSymbol & 1;
      aSymbol >>= 1;
      aPrice += m_Models[aModelIndex].GetPrice(aBit);
      aModelIndex = (aModelIndex << 1) | aBit;
    }
    return aPrice;
  }
};

/*
template <int aNumMoveBits, int aNumBitLevels>
class CReverseBitTreeEncoder: public CReverseBitTreeEncoder2<aNumMoveBits>
{
public:
  CReverseBitTreeEncoder() 
    { Create(aNumBitLevels); }
};
*/
////////////////////////////////
// CReverseBitTreeDecoder

template <int aNumMoveBits>
class CReverseBitTreeDecoder2
{
  CMyBitDecoder<aNumMoveBits> *m_Models;
  UINT32 m_NumBitLevels;
public:
  CReverseBitTreeDecoder2(): m_Models(0) { }
  ~CReverseBitTreeDecoder2() { delete []m_Models; }
  bool Create(UINT32 aNumBitLevels)
  {
    m_NumBitLevels = aNumBitLevels;
    m_Models = new CMyBitDecoder<aNumMoveBits>[1 << aNumBitLevels];
    return (m_Models != 0);
  }
  void Init()
  {
    UINT32 aNumModels = 1 << m_NumBitLevels;
    for(UINT32 i = 1; i < aNumModels; i++)
      m_Models[i].Init();
  }
  UINT32 Decode(CMyRangeDecoder *aRangeDecoder)
  {
    UINT32 aModelIndex = 1;
    UINT32 aSymbol = 0;
    RC_INIT_VAR
    for(UINT32 aBitIndex = 0; aBitIndex < m_NumBitLevels; aBitIndex++)
    {
      // UINT32 aBit = m_Models[aModelIndex].Decode(aRangeDecoder);
      // aModelIndex <<= 1;
      // aModelIndex += aBit;
      // aSymbol |= (aBit << aBitIndex);
      RC_GETBIT2(aNumMoveBits, m_Models[aModelIndex].m_Probability, aModelIndex, ; , aSymbol |= (1 << aBitIndex))
    }
    RC_FLUSH_VAR
    return aSymbol;
  };
};
////////////////////////////
// CReverseBitTreeDecoder2

template <int aNumMoveBits, UINT32 m_NumBitLevels>
class CReverseBitTreeDecoder
{
  CMyBitDecoder<aNumMoveBits> m_Models[1 << m_NumBitLevels];
public:
  void Init()
  {
    for(UINT32 i = 1; i < (1 << m_NumBitLevels); i++)
      m_Models[i].Init();
  }
  UINT32 Decode(CMyRangeDecoder *aRangeDecoder)
  {
    UINT32 aModelIndex = 1;
    UINT32 aSymbol = 0;
    RC_INIT_VAR
    for(UINT32 aBitIndex = 0; aBitIndex < m_NumBitLevels; aBitIndex++)
    {
      // UINT32 aBit = m_Models[aModelIndex].Decode(aRangeDecoder);
      // aModelIndex <<= 1;
      // aModelIndex += aBit;
      // aSymbol |= (aBit << aBitIndex);
      RC_GETBIT2(aNumMoveBits, m_Models[aModelIndex].m_Probability, aModelIndex, ; , aSymbol |= (1 << aBitIndex))
    }
    RC_FLUSH_VAR
    return aSymbol;
  }
};

/*
//////////////////////////
// CBitTreeEncoder2

template <int aNumMoveBits>
class CBitTreeEncoder2
{
  NCompression::NArithmetic::CBitEncoder<aNumMoveBits> *m_Models;
  UINT32 m_NumBitLevels;
public:
  bool Create(UINT32 aNumBitLevels)
  { 
    m_NumBitLevels = aNumBitLevels;
    m_Models = new NCompression::NArithmetic::CBitEncoder<aNumMoveBits>[1 << aNumBitLevels];
    return (m_Models != 0);
  }
  void Init()
  {
    UINT32 aNumModels = 1 << m_NumBitLevels;
    for(UINT32 i = 1; i < aNumModels; i++)
      m_Models[i].Init();
  }
  void Encode(CMyRangeEncoder *aRangeEncoder, UINT32 aSymbol)
  {
    UINT32 aModelIndex = 1;
    for (UINT32 aBitIndex = m_NumBitLevels; aBitIndex > 0 ;)
    {
      aBitIndex--;
      UINT32 aBit = (aSymbol >> aBitIndex ) & 1;
      m_Models[aModelIndex].Encode(aRangeEncoder, aBit);
      aModelIndex = (aModelIndex << 1) | aBit;
    }
  }
  UINT32 GetPrice(UINT32 aSymbol) const
  {
    UINT32 aPrice = 0;
    UINT32 aModelIndex = 1;
    for (UINT32 aBitIndex = m_NumBitLevels; aBitIndex > 0 ;)
    {
      aBitIndex--;
      UINT32 aBit = (aSymbol >> aBitIndex ) & 1;
      aPrice += m_Models[aModelIndex].GetPrice(aBit);
      aModelIndex = (aModelIndex << 1) + aBit;
    }
    return aPrice;
  }
};


//////////////////////////
// CBitTreeDecoder2

template <int aNumMoveBits>
class CBitTreeDecoder2
{
  NCompression::NArithmetic::CBitDecoder<aNumMoveBits> *m_Models;
  UINT32 m_NumBitLevels;
public:
  bool Create(UINT32 aNumBitLevels)
  { 
    m_NumBitLevels = aNumBitLevels;
    m_Models = new NCompression::NArithmetic::CBitDecoder<aNumMoveBits>[1 << aNumBitLevels];
    return (m_Models != 0);
  }
  void Init()
  {
    UINT32 aNumModels = 1 << m_NumBitLevels;
    for(UINT32 i = 1; i < aNumModels; i++)
      m_Models[i].Init();
  }
  UINT32 Decode(CMyRangeDecoder *aRangeDecoder)
  {
    UINT32 aModelIndex = 1;
    RC_INIT_VAR
    for(UINT32 aBitIndex = m_NumBitLevels; aBitIndex > 0; aBitIndex--)
    {
      // aModelIndex = (aModelIndex << 1) + m_Models[aModelIndex].Decode(aRangeDecoder);
      RC_GETBIT(aNumMoveBits, m_Models[aModelIndex].m_Probability, aModelIndex)
    }
    RC_FLUSH_VAR
    return aModelIndex - (1 << m_NumBitLevels);
  }
};
*/


#endif
