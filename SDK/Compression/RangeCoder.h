// Compression/RangeCoder.h
// This code is based on Eugene Shelwien's Rangecoder code

#pragma once

#ifndef __COMPRESSION_RANGECODER_H
#define __COMPRESSION_RANGECODER_H

#include "Stream/InByte.h"
#include "Stream/OutByte.h"

namespace NCompression {
namespace NArithmetic {

const UINT32 kNumTopBits = 24;
const UINT32 kTopValue = (1 << kNumTopBits);

class CRangeEncoder
{
  NStream::COutByte m_Stream;
  UINT64 m_Low;
  UINT32 m_Range;
  UINT32 m_FFNum;
  BYTE m_Cache;

public:
  void Init(ISequentialOutStream *aStream)
  {
    m_Stream.Init(aStream);
    m_Low = 0;
    m_Range = UINT32(-1);
    m_FFNum = 0;
    m_Cache = 0;
  }

  void FlushData()
  {
    // m_Low += 1; 
    for(int i = 0; i < 5; i++)
      ShiftLow();
  }

  HRESULT FlushStream()
    { return m_Stream.Flush();  }

  void ReleaseStream()
    { m_Stream.ReleaseStream(); }

  void Encode(UINT32 aStart, UINT32 aSize, UINT32 aTotal)
  {
    m_Low += aStart * (m_Range /= aTotal);
    m_Range *= aSize;
    while (m_Range < kTopValue)
    {
      m_Range <<= 8;
      ShiftLow();
    }
  }

  /*
  void EncodeDirectBitsDiv(UINT32 aValue, UINT32 aNumTotalBits)
  {
    m_Low += aValue * (m_Range >>= aNumTotalBits);
    Normalize();
  }
  
  void EncodeDirectBitsDiv2(UINT32 aValue, UINT32 aNumTotalBits)
  {
    if (aNumTotalBits <= kNumBottomBits)
      EncodeDirectBitsDiv(aValue, aNumTotalBits);
    else
    {
      EncodeDirectBitsDiv(aValue >> kNumBottomBits, (aNumTotalBits - kNumBottomBits));
      EncodeDirectBitsDiv(aValue & ((1 << kBottomValueBits) - 1), kNumBottomBits);
    }
  }
  */
  void ShiftLow()
  {
    if (m_Low < (UINT32)0xFF000000 || UINT32(m_Low >> 32) == 1) 
    {
      m_Stream.WriteByte(m_Cache + BYTE(m_Low >> 32));            
      for (;m_FFNum != 0; m_FFNum--) 
        m_Stream.WriteByte(0xFF + BYTE(m_Low >> 32));
      m_Cache = BYTE(UINT32(m_Low) >> 24);                      
    } 
    else 
      m_FFNum++;                               
    m_Low = UINT32(m_Low) << 8;                           
  }
  
  void EncodeDirectBits(UINT32 aValue, UINT32 aNumTotalBits)
  {
    for (int i = aNumTotalBits - 1; i >= 0; i--)
    {
      m_Range >>= 1;
      if (((aValue >> i) & 1) == 1)
        m_Low += m_Range;
      if (m_Range < kTopValue)
      {
        m_Range <<= 8;
        ShiftLow();
      }
    }
  }

  void EncodeBit(UINT32 aSize0, UINT32 aNumTotalBits, UINT32 aSymbol)
  {
    UINT32 aNewBound = (m_Range >> aNumTotalBits) * aSize0;
    if (aSymbol == 0)
      m_Range = aNewBound;
    else
    {
      m_Low += aNewBound;
      m_Range -= aNewBound;
    }
    while (m_Range < kTopValue)
    {
      m_Range <<= 8;
      ShiftLow();
    }
  }

  UINT64 GetProcessedSize() {  return m_Stream.GetProcessedSize() + m_FFNum; }
};

class CRangeDecoder
{
public:
  NStream::CInByte m_Stream;
  UINT32 m_Range;
  UINT32 m_Code;
  UINT32 m_Word;
  void Normalize()
  {
    while (m_Range < kTopValue)
    {
      m_Code = (m_Code << 8) | m_Stream.ReadByte();
      m_Range <<= 8;
    }
  }
  
  void Init(ISequentialInStream *aStream)
  {
    m_Stream.Init(aStream);
    m_Code = 0;
    m_Range = UINT32(-1);
    for(int i = 0; i < 5; i++)
      m_Code = (m_Code << 8) | m_Stream.ReadByte();
  }

  void ReleaseStream() { m_Stream.ReleaseStream(); }

  UINT32 GetThreshold(UINT32 aTotal)
  {
    return (m_Code) / ( m_Range /= aTotal);
  }

  void Decode(UINT32 aStart, UINT32 aSize, UINT32 aTotal)
  {
    m_Code -= aStart * m_Range;
    m_Range *= aSize;
    Normalize();
  }

  /*
  UINT32 DecodeDirectBitsDiv(UINT32 aNumTotalBits)
  {
    m_Range >>= aNumTotalBits;
    UINT32 aThreshold = m_Code / m_Range;
    m_Code -= aThreshold * m_Range;
    
    Normalize();
    return aThreshold;
  }

  UINT32 DecodeDirectBitsDiv2(UINT32 aNumTotalBits)
  {
    if (aNumTotalBits <= kNumBottomBits)
      return DecodeDirectBitsDiv(aNumTotalBits);
    UINT32 aResult = DecodeDirectBitsDiv(aNumTotalBits - kNumBottomBits) << kNumBottomBits;
    return (aResult | DecodeDirectBitsDiv(kNumBottomBits));
  }
  */

  UINT32 DecodeDirectBits(UINT32 aNumTotalBits)
  {
    UINT32 aRange = m_Range;
    UINT32 aCode = m_Code;        
    UINT32 aResult = 0;
    for (UINT32 i = aNumTotalBits; i > 0; i--)
    {
      aRange >>= 1;
      /*
      aResult <<= 1;
      if (aCode >= aRange)
      {
        aCode -= aRange;
        aResult |= 1;
      }
      */
      UINT32 t = (aCode - aRange) >> 31;
      aCode -= aRange & (t - 1);
      // aRange = aRangeTmp + ((aRange & 1) & (1 - t));
      aResult = (aResult << 1) | (1 - t);

      if (aRange < kTopValue)
      {
        aCode = (aCode << 8) | m_Stream.ReadByte();
        aRange <<= 8; 
      }
    }
    m_Range = aRange;
    m_Code = aCode;
    return aResult;
  }

  UINT32 DecodeBit(UINT32 aSize0, UINT32 aNumTotalBits)
  {
    UINT32 aNewBound = (m_Range >> aNumTotalBits) * aSize0;
    UINT32 aSymbol;
    if (m_Code < aNewBound)
    {
      aSymbol = 0;
      m_Range = aNewBound;
    }
    else
    {
      aSymbol = 1;
      m_Code -= aNewBound;
      m_Range -= aNewBound;
    }
    Normalize();
    return aSymbol;
  }

  UINT64 GetProcessedSize() {return m_Stream.GetProcessedSize(); }
};

}}

#endif
