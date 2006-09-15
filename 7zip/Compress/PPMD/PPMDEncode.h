// PPMDEncode.h
// This code is based on Dmitry Shkarin's PPMdH code

#ifndef __COMPRESS_PPMD_ENCODE_H
#define __COMPRESS_PPMD_ENCODE_H

#include "PPMDContext.h"

namespace NCompress {
namespace NPPMD {

struct CEncodeInfo: public CInfo
{

  void EncodeBinSymbol(int symbol, NRangeCoder::CEncoder *rangeEncoder)
  {
    PPM_CONTEXT::STATE& rs = MinContext->oneState();                   
    UInt16 &bs = GetBinSumm(rs, GetContextNoCheck(MinContext->Suffix)->NumStats);
    if (rs.Symbol == symbol) 
    {
      FoundState = &rs;
      rs.Freq = (Byte)(rs.Freq + (rs.Freq < 128 ? 1: 0));
      rangeEncoder->EncodeBit(bs, TOT_BITS, 0);
      bs = (UInt16)(bs + INTERVAL - GET_MEAN(bs, PERIOD_BITS, 2));
      PrevSuccess = 1;
      RunLength++;
    } 
    else 
    {
      rangeEncoder->EncodeBit(bs, TOT_BITS, 1);
      bs = (UInt16)(bs - GET_MEAN(bs, PERIOD_BITS, 2));
      InitEsc = ExpEscape[bs >> 10];
      NumMasked = 1;                        
      CharMask[rs.Symbol] = EscCount;
      PrevSuccess = 0;                      
      FoundState = NULL;
    }
  }

  void EncodeSymbol1(int symbol, NRangeCoder::CEncoder *rangeEncoder)
  {
    PPM_CONTEXT::STATE* p = GetStateNoCheck(MinContext->Stats);
    if (p->Symbol == symbol) 
    {
      PrevSuccess = (2 * (p->Freq) > MinContext->SummFreq);
      RunLength += PrevSuccess;
      rangeEncoder->Encode(0, p->Freq, MinContext->SummFreq);
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
        rangeEncoder->Encode(LoCnt, MinContext->SummFreq - LoCnt, MinContext->SummFreq);
        return;
      }
    }
    rangeEncoder->Encode(LoCnt, p->Freq, MinContext->SummFreq);
    update1(p);
  }

  void EncodeSymbol2(int symbol, NRangeCoder::CEncoder *rangeEncoder)
  {
    int hiCnt, i = MinContext->NumStats - NumMasked;
    UInt32 scale;
    SEE2_CONTEXT* psee2c = makeEscFreq2(i, scale);
    PPM_CONTEXT::STATE* p = GetStateNoCheck(MinContext->Stats) - 1;                       
    hiCnt = 0;
    do 
    {
      do { p++; } while (CharMask[p->Symbol] == EscCount);
      hiCnt += p->Freq;
      if (p->Symbol == symbol)            
        goto SYMBOL_FOUND;
      CharMask[p->Symbol] = EscCount;
    } 
    while ( --i );
    
    rangeEncoder->Encode(hiCnt, scale, hiCnt + scale);
    scale += hiCnt;
    
    psee2c->Summ = (UInt16)(psee2c->Summ + scale);
    NumMasked = MinContext->NumStats;
    return;
SYMBOL_FOUND:
    
    UInt32 highCount = hiCnt;
    UInt32 lowCount = highCount - p->Freq;
    if ( --i ) 
    {
      PPM_CONTEXT::STATE* p1 = p;
      do 
      {
        do { p1++; } while (CharMask[p1->Symbol] == EscCount);
        hiCnt += p1->Freq;
      } 
      while ( --i );
    }
    // SubRange.scale += hiCnt;
    scale += hiCnt;
    rangeEncoder->Encode(lowCount, highCount - lowCount, scale);
    psee2c->update();                       
    update2(p);
  }

  void EncodeSymbol(int c, NRangeCoder::CEncoder *rangeEncoder)
  {
    if (MinContext->NumStats != 1) 
      EncodeSymbol1(c, rangeEncoder);   
    else 
      EncodeBinSymbol(c, rangeEncoder); 
    while ( !FoundState ) 
    {
      do 
      {
        OrderFall++;                
        MinContext = GetContext(MinContext->Suffix);
        if (MinContext == 0) 
          return; //  S_OK;
      } 
      while (MinContext->NumStats == NumMasked);
      EncodeSymbol2(c, rangeEncoder);   
    }
    NextContext();
  }

};
}}

#endif
