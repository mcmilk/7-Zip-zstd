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
  NStream::COutByte Stream;
  UINT64 Low;
  UINT32 Range;
  UINT32 _ffNum;
  BYTE _cache;

public:
  void Init(ISequentialOutStream *stream)
  {
    Stream.Init(stream);
    Low = 0;
    Range = UINT32(-1);
    _ffNum = 0;
    _cache = 0;
  }

  void FlushData()
  {
    // Low += 1; 
    for(int i = 0; i < 5; i++)
      ShiftLow();
  }

  HRESULT FlushStream()
    { return Stream.Flush();  }

  void ReleaseStream()
    { Stream.ReleaseStream(); }

  void Encode(UINT32 start, UINT32 size, UINT32 total)
  {
    Low += start * (Range /= total);
    Range *= size;
    while (Range < kTopValue)
    {
      Range <<= 8;
      ShiftLow();
    }
  }

  /*
  void EncodeDirectBitsDiv(UINT32 value, UINT32 numTotalBits)
  {
    Low += value * (Range >>= numTotalBits);
    Normalize();
  }
  
  void EncodeDirectBitsDiv2(UINT32 value, UINT32 numTotalBits)
  {
    if (numTotalBits <= kNumBottomBits)
      EncodeDirectBitsDiv(value, numTotalBits);
    else
    {
      EncodeDirectBitsDiv(value >> kNumBottomBits, (numTotalBits - kNumBottomBits));
      EncodeDirectBitsDiv(value & ((1 << kBottomValueBits) - 1), kNumBottomBits);
    }
  }
  */
  void ShiftLow()
  {
    if (Low < (UINT32)0xFF000000 || UINT32(Low >> 32) == 1) 
    {
      Stream.WriteByte(_cache + BYTE(Low >> 32));            
      for (;_ffNum != 0; _ffNum--) 
        Stream.WriteByte(0xFF + BYTE(Low >> 32));
      _cache = BYTE(UINT32(Low) >> 24);                      
    } 
    else 
      _ffNum++;                               
    Low = UINT32(Low) << 8;                           
  }
  
  void EncodeDirectBits(UINT32 value, UINT32 numTotalBits)
  {
    for (int i = numTotalBits - 1; i >= 0; i--)
    {
      Range >>= 1;
      if (((value >> i) & 1) == 1)
        Low += Range;
      if (Range < kTopValue)
      {
        Range <<= 8;
        ShiftLow();
      }
    }
  }

  void EncodeBit(UINT32 size0, UINT32 numTotalBits, UINT32 symbol)
  {
    UINT32 newBound = (Range >> numTotalBits) * size0;
    if (symbol == 0)
      Range = newBound;
    else
    {
      Low += newBound;
      Range -= newBound;
    }
    while (Range < kTopValue)
    {
      Range <<= 8;
      ShiftLow();
    }
  }

  UINT64 GetProcessedSize() {  return Stream.GetProcessedSize() + _ffNum; }
};

class CRangeDecoder
{
public:
  NStream::CInByte Stream;
  UINT32 Range;
  UINT32 Code;
  // UINT32 m_Word;
  void Normalize()
  {
    while (Range < kTopValue)
    {
      Code = (Code << 8) | Stream.ReadByte();
      Range <<= 8;
    }
  }
  
  void Init(ISequentialInStream *stream)
  {
    Stream.Init(stream);
    Code = 0;
    Range = UINT32(-1);
    for(int i = 0; i < 5; i++)
      Code = (Code << 8) | Stream.ReadByte();
  }

  void ReleaseStream() { Stream.ReleaseStream(); }

  UINT32 GetThreshold(UINT32 total)
  {
    return (Code) / ( Range /= total);
  }

  void Decode(UINT32 start, UINT32 size, UINT32 total)
  {
    Code -= start * Range;
    Range *= size;
    Normalize();
  }

  /*
  UINT32 DecodeDirectBitsDiv(UINT32 numTotalBits)
  {
    Range >>= numTotalBits;
    UINT32 threshold = Code / Range;
    Code -= threshold * Range;
    
    Normalize();
    return threshold;
  }

  UINT32 DecodeDirectBitsDiv2(UINT32 numTotalBits)
  {
    if (numTotalBits <= kNumBottomBits)
      return DecodeDirectBitsDiv(numTotalBits);
    UINT32 result = DecodeDirectBitsDiv(numTotalBits - kNumBottomBits) << kNumBottomBits;
    return (result | DecodeDirectBitsDiv(kNumBottomBits));
  }
  */

  UINT32 DecodeDirectBits(UINT32 numTotalBits)
  {
    UINT32 range = Range;
    UINT32 code = Code;        
    UINT32 result = 0;
    for (UINT32 i = numTotalBits; i > 0; i--)
    {
      range >>= 1;
      /*
      result <<= 1;
      if (code >= range)
      {
        code -= range;
        result |= 1;
      }
      */
      UINT32 t = (code - range) >> 31;
      code -= range & (t - 1);
      // range = rangeTmp + ((range & 1) & (1 - t));
      result = (result << 1) | (1 - t);

      if (range < kTopValue)
      {
        code = (code << 8) | Stream.ReadByte();
        range <<= 8; 
      }
    }
    Range = range;
    Code = code;
    return result;
  }

  UINT32 DecodeBit(UINT32 size0, UINT32 numTotalBits)
  {
    UINT32 newBound = (Range >> numTotalBits) * size0;
    UINT32 symbol;
    if (Code < newBound)
    {
      symbol = 0;
      Range = newBound;
    }
    else
    {
      symbol = 1;
      Code -= newBound;
      Range -= newBound;
    }
    Normalize();
    return symbol;
  }

  UINT64 GetProcessedSize() {return Stream.GetProcessedSize(); }
};

}}

#endif
