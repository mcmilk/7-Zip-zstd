// LiteralCoder.h

#pragma once

#ifndef __LITERALCODER_H
#define __LITERALCODER_H

#include "Compression/AriBitCoder.h"

#include "RCDefs.h"

namespace NLiteral {

const kNumMoveBits = 5;

class CEncoder2
{
  CMyBitEncoder<kNumMoveBits> m_Encoders[3][1 << 8];
public:
  void Init();
  void Encode(CMyRangeEncoder *aRangeEncoder, bool aMatchMode, BYTE aMatchByte, BYTE aSymbol);
  UINT32 GetPrice(bool aMatchMode, BYTE aMatchByte, BYTE aSymbol) const;
};

class CDecoder2
{
  CMyBitDecoder<kNumMoveBits> m_Decoders[3][1 << 8];
public:
  void Init()
  {
    for (int i = 0; i < 3; i++)
      for (int j = 1; j < (1 << 8); j++)
        m_Decoders[i][j].Init();
  }

  BYTE DecodeNormal(CMyRangeDecoder *aRangeDecoder)
  {
    UINT32 aSymbol = 1;
    RC_INIT_VAR
    do
    {
      // aSymbol = (aSymbol << 1) | m_Decoders[0][aSymbol].Decode(aRangeDecoder);
      RC_GETBIT(kNumMoveBits, m_Decoders[0][aSymbol].m_Probability, aSymbol)
    }
    while (aSymbol < 0x100);
    RC_FLUSH_VAR
    return aSymbol;
  }

  BYTE DecodeWithMatchByte(CMyRangeDecoder *aRangeDecoder, BYTE aMatchByte)
  {
    UINT32 aSymbol = 1;
    RC_INIT_VAR
    do
    {
      UINT32 aMatchBit = (aMatchByte >> 7) & 1;
      aMatchByte <<= 1;
      // UINT32 aBit = m_Decoders[1 + aMatchBit][aSymbol].Decode(aRangeDecoder);
      // aSymbol = (aSymbol << 1) | aBit;
      UINT32 aBit;
      RC_GETBIT2(kNumMoveBits, m_Decoders[1 + aMatchBit][aSymbol].m_Probability, aSymbol, 
          aBit = 0, aBit = 1)
      if (aMatchBit != aBit)
      {
        while (aSymbol < 0x100)
        {
          // aSymbol = (aSymbol << 1) | m_Decoders[0][aSymbol].Decode(aRangeDecoder);
          RC_GETBIT(kNumMoveBits, m_Decoders[0][aSymbol].m_Probability, aSymbol)
        }
        break;
      }
    }
    while (aSymbol < 0x100);
    RC_FLUSH_VAR
    return aSymbol;
  }
};

/*
const UINT32 kNumPrevByteBits = 1;
const UINT32 kNumPrevByteStates =  (1 << kNumPrevByteBits);

inline UINT32 GetLiteralState(BYTE aPrevByte)
  { return (aPrevByte >> (8 - kNumPrevByteBits)); }
*/

class CEncoder
{
  CEncoder2 *m_Coders;
  UINT32 m_NumPrevBits;
  UINT32 m_NumPosBits;
  UINT32 m_PosMask;
public:
  CEncoder(): m_Coders(0) {}
  ~CEncoder()  { Free(); }
  void Free()
  { 
    delete []m_Coders;
    m_Coders = 0;
  }
  void Create(UINT32 aNumPosBits, UINT32 aNumPrevBits)
  {
    Free();
    m_NumPosBits = aNumPosBits;
    m_PosMask = (1 << aNumPosBits) - 1;
    m_NumPrevBits = aNumPrevBits;
    UINT32 aNumStates = 1 << (m_NumPrevBits + m_NumPosBits);
    m_Coders = new CEncoder2[aNumStates];
  }
  void Init()
  {
    UINT32 aNumStates = 1 << (m_NumPrevBits + m_NumPosBits);
    for (UINT32 i = 0; i < aNumStates; i++)
      m_Coders[i].Init();
  }
  UINT32 GetState(UINT32 aPos, BYTE aPrevByte) const
    { return ((aPos & m_PosMask) << m_NumPrevBits) + (aPrevByte >> (8 - m_NumPrevBits)); }
  void Encode(CMyRangeEncoder *aRangeEncoder, UINT32 aPos, BYTE aPrevByte, 
      bool aMatchMode, BYTE aMatchByte, BYTE aSymbol)
    { m_Coders[GetState(aPos, aPrevByte)].Encode(aRangeEncoder, aMatchMode, 
          aMatchByte, aSymbol); }
  UINT32 GetPrice(UINT32 aPos, BYTE aPrevByte, bool aMatchMode, BYTE aMatchByte, BYTE aSymbol) const
    { return m_Coders[GetState(aPos, aPrevByte)].GetPrice(aMatchMode, aMatchByte, aSymbol); }
};

class CDecoder
{
  CDecoder2 *m_Coders;
  UINT32 m_NumPrevBits;
  UINT32 m_NumPosBits;
  UINT32 m_PosMask;
public:
  CDecoder(): m_Coders(0) {}
  ~CDecoder()  { Free(); }
  void Free()
  { 
    delete []m_Coders;
    m_Coders = 0;
  }
  void Create(UINT32 aNumPosBits, UINT32 aNumPrevBits)
  {
    Free();
    m_NumPosBits = aNumPosBits;
    m_PosMask = (1 << aNumPosBits) - 1;
    m_NumPrevBits = aNumPrevBits;
    UINT32 aNumStates = 1 << (m_NumPrevBits + m_NumPosBits);
    m_Coders = new CDecoder2[aNumStates];
  }
  void Init()
  {
    UINT32 aNumStates = 1 << (m_NumPrevBits + m_NumPosBits);
    for (UINT32 i = 0; i < aNumStates; i++)
      m_Coders[i].Init();
  }
  UINT32 GetState(UINT32 aPos, BYTE aPrevByte) const
    { return ((aPos & m_PosMask) << m_NumPrevBits) + (aPrevByte >> (8 - m_NumPrevBits)); }
  BYTE DecodeNormal(CMyRangeDecoder *aRangeDecoder, UINT32 aPos, BYTE aPrevByte)
    { return m_Coders[GetState(aPos, aPrevByte)].DecodeNormal(aRangeDecoder); }
  BYTE DecodeWithMatchByte(CMyRangeDecoder *aRangeDecoder, UINT32 aPos, BYTE aPrevByte, BYTE aMatchByte)
    { return m_Coders[GetState(aPos, aPrevByte)].DecodeWithMatchByte(aRangeDecoder, aMatchByte); }
};

}

#endif
