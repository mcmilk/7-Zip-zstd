// Encode.h
// This code is based on Dmitry Shkarin's PPMdH code

#pragma once

#ifndef __COMPRESS_PPM_PPMD_ENCODE_H
#define __COMPRESS_PPM_PPMD_ENCODE_H

#include "Context.h"
#include "AriConst.h"

namespace NCompress {
namespace NPPMD {

struct CEncodeInfo: public CInfo
{

  void EncodeBinSymbol(int symbol, CMyRangeEncoder *aRangeEncoder)
  {
    PPM_CONTEXT::STATE& rs = MinContext->oneState();                   
    WORD &bs = GetBinSumm(rs, MinContext->Suffix->NumStats);
    if (rs.Symbol == symbol) 
    {
      FoundState = &rs;
      rs.Freq += (rs.Freq < 128);
      aRangeEncoder->EncodeBit(bs, TOT_BITS, 0);
      bs += UINT16(INTERVAL-GET_MEAN(bs,PERIOD_BITS, 2));
      PrevSuccess = 1;
      RunLength++;
    } 
    else 
    {
      aRangeEncoder->EncodeBit(bs, TOT_BITS, 1);
      bs -= UINT16(GET_MEAN(bs,PERIOD_BITS, 2));
      InitEsc = ExpEscape[bs >> 10];
      NumMasked = 1;                        
      CharMask[rs.Symbol] = EscCount;
      PrevSuccess = 0;                      
      FoundState = NULL;
    }
  }

  void EncodeSymbol1(int symbol, CMyRangeEncoder *aRangeEncoder)
  {
    PPM_CONTEXT::STATE* p = MinContext->Stats;
    if (p->Symbol == symbol) 
    {
      PrevSuccess = (2 * (p->Freq) > MinContext->SummFreq);
      RunLength += PrevSuccess;
      aRangeEncoder->Encode(0, MinContext->Stats->Freq, MinContext->SummFreq);
      (FoundState = p)->Freq += 4;          
      MinContext->SummFreq += 4;
      if (p->Freq > MAX_FREQ)             
        rescale();
      return;
    }
    PrevSuccess = 0;
    int LoCnt = p->Freq, i = MinContext->NumStats - 1;
    while ((++p)->Symbol != symbol) 
    {
      LoCnt += p->Freq;
      if (--i == 0) 
      {
        HiBitsFlag = HB2Flag[FoundState->Symbol];
        CharMask[p->Symbol] = EscCount;
        i=(NumMasked = MinContext->NumStats)-1;       
        FoundState = NULL;
        do { CharMask[(--p)->Symbol] = EscCount; } while ( --i );
        aRangeEncoder->Encode(LoCnt, MinContext->SummFreq - LoCnt, MinContext->SummFreq);
        return;
      }
    }
    aRangeEncoder->Encode(LoCnt, p->Freq, MinContext->SummFreq);
    update1(p);
  }

  void EncodeSymbol2(int symbol, CMyRangeEncoder *aRangeEncoder)
  {
    int HiCnt, i = MinContext->NumStats - NumMasked;
    UINT32 aScale;
    SEE2_CONTEXT* psee2c = makeEscFreq2(i, aScale);
    PPM_CONTEXT::STATE* p = MinContext->Stats - 1;                       
    HiCnt = 0;
    do 
    {
      do { p++; } while (CharMask[p->Symbol] == EscCount);
      HiCnt += p->Freq;
      if (p->Symbol == symbol)            
        goto SYMBOL_FOUND;
      CharMask[p->Symbol] = EscCount;
    } 
    while ( --i );
    
    aRangeEncoder->Encode(HiCnt, aScale, HiCnt + aScale);
    aScale += HiCnt;
    
    psee2c->Summ += aScale;         
    NumMasked = MinContext->NumStats;
    return;
SYMBOL_FOUND:
    
    UINT32 aHighCount = HiCnt;
    UINT32 aLowCount = aHighCount - p->Freq;
    if ( --i ) 
    {
      PPM_CONTEXT::STATE* p1 = p;
      do 
      {
        do { p1++; } while (CharMask[p1->Symbol] == EscCount);
        HiCnt += p1->Freq;
      } 
      while ( --i );
    }
    // SubRange.scale += HiCnt;
    aScale += HiCnt;
    aRangeEncoder->Encode(aLowCount, aHighCount - aLowCount, aScale);
    psee2c->update();                       
    update2(p);
  }

  void EncodeSymbol(int c, CMyRangeEncoder *aRangeEncoder)
  {
    if (MinContext->NumStats != 1) 
      EncodeSymbol1(c, aRangeEncoder);   
    else 
      EncodeBinSymbol(c, aRangeEncoder); 
    while ( !FoundState ) 
    {
      do 
      {
        OrderFall++;                
        MinContext = MinContext->Suffix;
        // if ( !MinContext ) return; //  S_OK;
      } 
      while (MinContext->NumStats == NumMasked);
      EncodeSymbol2(c, aRangeEncoder);   
    }
    NextContext();
  }

};
}}

#endif
