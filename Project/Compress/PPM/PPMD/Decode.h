// Decode.h
// This code is based on Dmitry Shkarin's PPMdH code

#pragma once

#ifndef __COMPRESS_PPM_PPMD_DECODE_H
#define __COMPRESS_PPM_PPMD_DECODE_H

#include "Context.h"
#include "AriConst.h"

namespace NCompress {
namespace NPPMD {

struct CDecodeInfo: public CInfo
{

  void DecodeBinSymbol(CMyRangeDecoder *aRangeDecoder)
  {
    PPM_CONTEXT::STATE& rs = MinContext->oneState();                   
    WORD& bs = GetBinSumm(rs, MinContext->Suffix->NumStats);
    if (aRangeDecoder->DecodeBit(bs, TOT_BITS) == 0) 
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

  void DecodeSymbol1(CMyRangeDecoder *aRangeDecoder)
  {
    PPM_CONTEXT::STATE* p = MinContext->Stats;
    int i, count, HiCnt;
    if ((count = aRangeDecoder->GetThreshold(MinContext->SummFreq)) < (HiCnt = p->Freq)) 
    {
      PrevSuccess = (2 * HiCnt > MinContext->SummFreq);
      RunLength += PrevSuccess;
      aRangeDecoder->Decode(0, MinContext->Stats->Freq, MinContext->SummFreq);
      (FoundState = p)->Freq=(HiCnt += 4);  
      MinContext->SummFreq += 4;
      if (HiCnt > MAX_FREQ)
        rescale();
      return;
    }
    PrevSuccess = 0;
    i = MinContext->NumStats - 1;
    while ((HiCnt += (++p)->Freq) <= count)
      if (--i == 0) 
      {
        HiBitsFlag = HB2Flag[FoundState->Symbol];
        aRangeDecoder->Decode(HiCnt, MinContext->SummFreq - HiCnt, MinContext->SummFreq);
        CharMask[p->Symbol] = EscCount;
        i = (NumMasked = MinContext->NumStats)-1;       
        FoundState = NULL;
        do { CharMask[(--p)->Symbol] = EscCount; } while ( --i );
        return;
      }
    aRangeDecoder->Decode(HiCnt - p->Freq, p->Freq, MinContext->SummFreq);
    update1(p);
  }


  void DecodeSymbol2(CMyRangeDecoder *aRangeDecoder)
  {
    int count, HiCnt, i = MinContext->NumStats - NumMasked;
    UINT32 aFreqSum;
    SEE2_CONTEXT* psee2c = makeEscFreq2(i, aFreqSum);
    PPM_CONTEXT::STATE* ps[256], ** pps = ps, * p = MinContext->Stats-1;
    HiCnt = 0;
    do 
    {
      do { p++; } while (CharMask[p->Symbol] == EscCount);
      HiCnt += p->Freq;                   
      *pps++ = p;
    } 
    while ( --i );
    
    aFreqSum += HiCnt;
    count = aRangeDecoder->GetThreshold(aFreqSum);
    
    p = *(pps = ps);
    if (count < HiCnt) 
    {
      HiCnt = 0;
      while ((HiCnt += p->Freq) <= count) 
        p=*++pps;
      aRangeDecoder->Decode(HiCnt - p->Freq, p->Freq, aFreqSum);
      
      psee2c->update();                   
      update2(p);
    } 
    else 
    {
      aRangeDecoder->Decode(HiCnt, aFreqSum - HiCnt, aFreqSum);
      
      i = MinContext->NumStats - NumMasked;               
      pps--;
      do { CharMask[(*++pps)->Symbol] = EscCount; } while ( --i );
      psee2c->Summ += aFreqSum;     
      NumMasked = MinContext->NumStats;
    }
  }

  int DecodeSymbol(CMyRangeDecoder *aRangeDecoder)
  {
    if (MinContext->NumStats != 1)                        
      DecodeSymbol1(aRangeDecoder);
    else                                
      DecodeBinSymbol(aRangeDecoder);
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
      DecodeSymbol2(aRangeDecoder);    
    }
    BYTE aSymbol = FoundState->Symbol;
    NextContext();
    return aSymbol;
  }
};

}}

#endif
