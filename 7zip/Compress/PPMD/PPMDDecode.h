// Decode.h
// This code is based on Dmitry Shkarin's PPMdH code

#pragma once

#ifndef __COMPRESS_PPM_PPMD_DECODE_H
#define __COMPRESS_PPM_PPMD_DECODE_H

#include "PPMDContext.h"

namespace NCompress {
namespace NPPMD {

struct CDecodeInfo: public CInfo
{
  void DecodeBinSymbol(NRangeCoder::CDecoder *rangeDecoder)
  {
    PPM_CONTEXT::STATE& rs = MinContext->oneState();                   
    WORD& bs = GetBinSumm(rs, MinContext->Suffix->NumStats);
    if (rangeDecoder->DecodeBit(bs, TOT_BITS) == 0) 
    {
      FoundState = &rs;
      rs.Freq += (rs.Freq < 128);
      bs += UINT16(INTERVAL-GET_MEAN(bs,PERIOD_BITS,2));
      PrevSuccess = 1;
      RunLength++;
    } 
    else 
    {
      bs -= UINT16(GET_MEAN(bs,PERIOD_BITS,2));
      InitEsc = ExpEscape[bs >> 10];
      NumMasked = 1;                        
      CharMask[rs.Symbol] = EscCount;
      PrevSuccess = 0;                      
      FoundState = NULL;
    }
  }

  void DecodeSymbol1(NRangeCoder::CDecoder *rangeDecoder)
  {
    PPM_CONTEXT::STATE* p = MinContext->Stats;
    int i, count, hiCnt;
    if ((count = rangeDecoder->GetThreshold(MinContext->SummFreq)) < (hiCnt = p->Freq)) 
    {
      PrevSuccess = (2 * hiCnt > MinContext->SummFreq);
      RunLength += PrevSuccess;
      rangeDecoder->Decode(0, MinContext->Stats->Freq, MinContext->SummFreq);
      (FoundState = p)->Freq=(hiCnt += 4);  
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
        rangeDecoder->Decode(hiCnt, MinContext->SummFreq - hiCnt, MinContext->SummFreq);
        CharMask[p->Symbol] = EscCount;
        i = (NumMasked = MinContext->NumStats)-1;       
        FoundState = NULL;
        do { CharMask[(--p)->Symbol] = EscCount; } while ( --i );
        return;
      }
    rangeDecoder->Decode(hiCnt - p->Freq, p->Freq, MinContext->SummFreq);
    update1(p);
  }


  void DecodeSymbol2(NRangeCoder::CDecoder *rangeDecoder)
  {
    int count, hiCnt, i = MinContext->NumStats - NumMasked;
    UINT32 freqSum;
    SEE2_CONTEXT* psee2c = makeEscFreq2(i, freqSum);
    PPM_CONTEXT::STATE* ps[256], ** pps = ps, * p = MinContext->Stats-1;
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
      rangeDecoder->Decode(hiCnt - p->Freq, p->Freq, freqSum);
      
      psee2c->update();                   
      update2(p);
    } 
    else 
    {
      rangeDecoder->Decode(hiCnt, freqSum - hiCnt, freqSum);
      
      i = MinContext->NumStats - NumMasked;               
      pps--;
      do { CharMask[(*++pps)->Symbol] = EscCount; } while ( --i );
      psee2c->Summ += freqSum;     
      NumMasked = MinContext->NumStats;
    }
  }

  int DecodeSymbol(NRangeCoder::CDecoder *rangeDecoder)
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
        MinContext = MinContext->Suffix;
        if ( !MinContext )          
          return -1;
      } 
      while (MinContext->NumStats == NumMasked);
      DecodeSymbol2(rangeDecoder);    
    }
    BYTE symbol = FoundState->Symbol;
    NextContext();
    return symbol;
  }
};

}}

#endif
