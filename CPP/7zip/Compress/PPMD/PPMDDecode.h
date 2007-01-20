// PPMDDecode.h
// This code is based on Dmitry Shkarin's PPMdH code

#ifndef __COMPRESS_PPMD_DECODE_H
#define __COMPRESS_PPMD_DECODE_H

#include "PPMDContext.h"

namespace NCompress {
namespace NPPMD {

class CRangeDecoderVirt
{
public:
  virtual UInt32 GetThreshold(UInt32 total) = 0;
  virtual void Decode(UInt32 start, UInt32 size) = 0;
  virtual UInt32 DecodeBit(UInt32 size0, UInt32 numTotalBits) = 0;
};

typedef NRangeCoder::CDecoder CRangeDecoderMy;

class CRangeDecoder:public CRangeDecoderVirt, public CRangeDecoderMy
{
  UInt32 GetThreshold(UInt32 total) { return CRangeDecoderMy::GetThreshold(total); }
  void Decode(UInt32 start, UInt32 size) { CRangeDecoderMy::Decode(start, size); }
  UInt32 DecodeBit(UInt32 size0, UInt32 numTotalBits) { return CRangeDecoderMy::DecodeBit(size0, numTotalBits); }
};

struct CDecodeInfo: public CInfo
{
  void DecodeBinSymbol(CRangeDecoderVirt *rangeDecoder)
  {
    PPM_CONTEXT::STATE& rs = MinContext->oneState();                   
    UInt16& bs = GetBinSumm(rs, GetContextNoCheck(MinContext->Suffix)->NumStats);
    if (rangeDecoder->DecodeBit(bs, TOT_BITS) == 0) 
    {
      FoundState = &rs;
      rs.Freq = (Byte)(rs.Freq + (rs.Freq < 128 ? 1: 0));
      bs = (UInt16)(bs + INTERVAL - GET_MEAN(bs, PERIOD_BITS, 2));
      PrevSuccess = 1;
      RunLength++;
    } 
    else 
    {
      bs = (UInt16)(bs - GET_MEAN(bs, PERIOD_BITS, 2));
      InitEsc = ExpEscape[bs >> 10];
      NumMasked = 1;                        
      CharMask[rs.Symbol] = EscCount;
      PrevSuccess = 0;                      
      FoundState = NULL;
    }
  }

  void DecodeSymbol1(CRangeDecoderVirt *rangeDecoder)
  {
    PPM_CONTEXT::STATE* p = GetStateNoCheck(MinContext->Stats);
    int i, count, hiCnt;
    if ((count = rangeDecoder->GetThreshold(MinContext->SummFreq)) < (hiCnt = p->Freq)) 
    {
      PrevSuccess = (2 * hiCnt > MinContext->SummFreq);
      RunLength += PrevSuccess;
      rangeDecoder->Decode(0, p->Freq); // MinContext->SummFreq);
      (FoundState = p)->Freq = (Byte)(hiCnt += 4);  
      MinContext->SummFreq += 4;
      if (hiCnt > MAX_FREQ)
        rescale();
      return;
    }
    PrevSuccess = 0;
    i = MinContext->NumStats - 1;
    while ((hiCnt += (++p)->Freq) <= count)
      if (--i == 0) 
      {
        HiBitsFlag = HB2Flag[FoundState->Symbol];
        rangeDecoder->Decode(hiCnt, MinContext->SummFreq - hiCnt); // , MinContext->SummFreq);
        CharMask[p->Symbol] = EscCount;
        i = (NumMasked = MinContext->NumStats)-1;       
        FoundState = NULL;
        do { CharMask[(--p)->Symbol] = EscCount; } while ( --i );
        return;
      }
    rangeDecoder->Decode(hiCnt - p->Freq, p->Freq); // , MinContext->SummFreq);
    update1(p);
  }


  void DecodeSymbol2(CRangeDecoderVirt *rangeDecoder)
  {
    int count, hiCnt, i = MinContext->NumStats - NumMasked;
    UInt32 freqSum;
    SEE2_CONTEXT* psee2c = makeEscFreq2(i, freqSum);
    PPM_CONTEXT::STATE* ps[256], ** pps = ps, * p = GetStateNoCheck(MinContext->Stats)-1;
    hiCnt = 0;
    do 
    {
      do { p++; } while (CharMask[p->Symbol] == EscCount);
      hiCnt += p->Freq;                   
      *pps++ = p;
    } 
    while ( --i );
    
    freqSum += hiCnt;
    count = rangeDecoder->GetThreshold(freqSum);
    
    p = *(pps = ps);
    if (count < hiCnt) 
    {
      hiCnt = 0;
      while ((hiCnt += p->Freq) <= count) 
        p=*++pps;
      rangeDecoder->Decode(hiCnt - p->Freq, p->Freq); // , freqSum);
      
      psee2c->update();                   
      update2(p);
    } 
    else 
    {
      rangeDecoder->Decode(hiCnt, freqSum - hiCnt); // , freqSum);
      
      i = MinContext->NumStats - NumMasked;               
      pps--;
      do { CharMask[(*++pps)->Symbol] = EscCount; } while ( --i );
      psee2c->Summ = (UInt16)(psee2c->Summ + freqSum);     
      NumMasked = MinContext->NumStats;
    }
  }

  int DecodeSymbol(CRangeDecoderVirt *rangeDecoder)
  {
    if (MinContext->NumStats != 1)                        
      DecodeSymbol1(rangeDecoder);
    else                                
      DecodeBinSymbol(rangeDecoder);
    while ( !FoundState ) 
    {
      do 
      {
        OrderFall++;                
        MinContext = GetContext(MinContext->Suffix);
        if (MinContext == 0)          
          return -1;
      } 
      while (MinContext->NumStats == NumMasked);
      DecodeSymbol2(rangeDecoder);    
    }
    Byte symbol = FoundState->Symbol;
    NextContext();
    return symbol;
  }
};

}}

#endif
