// Compress/PPM/PPMD/Context.h
// This code is based on Dmitry Shkarin's PPMdH code

#pragma once

#ifndef __COMPRESS_PPM_PPMD_CONTEXT_H
#define __COMPRESS_PPM_PPMD_CONTEXT_H

#include "SubAlloc.h"
#include "SubAlloc.h"

#include "Common/Types.h"

#include "Compression/RangeCoder.h"

namespace NCompress {
namespace NPPMD {

const int INT_BITS=7, PERIOD_BITS=7, TOT_BITS=INT_BITS+PERIOD_BITS,
        INTERVAL=1 << INT_BITS, BIN_SCALE=1 << TOT_BITS, MAX_FREQ=124;

#pragma pack(1)
struct SEE2_CONTEXT 
{   // SEE-contexts for PPM-contexts with masked symbols
  WORD Summ;
  BYTE Shift, Count;
  void init(int InitVal) { Summ=InitVal << (Shift=PERIOD_BITS-4); Count=4; }
  UINT getMean() 
  {
    UINT RetVal=(Summ >> Shift);        
    Summ -= RetVal;
    return RetVal+(RetVal == 0);
  }
  void update() 
  {
    if (Shift < PERIOD_BITS && --Count == 0) 
    {
      Summ += Summ;                   
      Count = 3 << Shift++;
    }
  }
};

struct PPM_CONTEXT 
{
  WORD NumStats,SummFreq;                     // sizeof(WORD) > sizeof(BYTE)
  struct STATE { BYTE Symbol, Freq; PPM_CONTEXT* Successor; } _PACK_ATTR * Stats;
  PPM_CONTEXT* Suffix;
  
  PPM_CONTEXT* createChild(CSubAllocator &aSubAllocator, STATE* pStats, STATE& FirstState)
  {
    PPM_CONTEXT* pc = (PPM_CONTEXT*) aSubAllocator.AllocContext();
    if ( pc ) 
    {
      pc->NumStats = 1;                     
      pc->oneState() = FirstState;
      pc->Suffix = this;                    
      pStats->Successor = pc;
    }
    return pc;
  }

  STATE& oneState() const { return (STATE&) SummFreq; }
};
#pragma pack()

/////////////////////////////////

const WORD InitBinEsc[] =
  {0x3CDD, 0x1F3F, 0x59BF, 0x48F3, 0x64A1, 0x5ABC, 0x6632, 0x6051};

struct CInfo
{
  CSubAllocator m_SubAllocator;
  SEE2_CONTEXT _PACK_ATTR SEE2Cont[25][16], DummySEE2Cont;
  PPM_CONTEXT _PACK_ATTR * MinContext, * MaxContext;

  PPM_CONTEXT::STATE* FoundState;      // found next state transition
  int NumMasked, InitEsc, OrderFall, RunLength, InitRL, MaxOrder;
  BYTE CharMask[256], NS2Indx[256], NS2BSIndx[256], HB2Flag[256];
  BYTE EscCount, PrintCount, PrevSuccess, HiBitsFlag;
  WORD BinSumm[128][64];               // binary SEE-contexts

  WORD &GetBinSumm(const PPM_CONTEXT::STATE &rs, int aNumStates)
  {
    HiBitsFlag = HB2Flag[FoundState->Symbol];
    return BinSumm[rs.Freq - 1][
         PrevSuccess + NS2BSIndx[aNumStates - 1] +
         HiBitsFlag + 2 * HB2Flag[rs.Symbol] + 
         ((RunLength >> 26) & 0x20)];
  }

  void RestartModelRare()
  {
    int i, k, m;
    memset(CharMask,0,sizeof(CharMask));
    m_SubAllocator.InitSubAllocator();                     
    InitRL = -((MaxOrder < 12) ? MaxOrder : 12) - 1;
    MinContext = MaxContext = (PPM_CONTEXT*) m_SubAllocator.AllocContext();
    MinContext->Suffix = NULL;                
    OrderFall = MaxOrder;
    MinContext->SummFreq = (MinContext->NumStats = 256) + 1;
    FoundState = MinContext->Stats = (PPM_CONTEXT::STATE*) m_SubAllocator.AllocUnits(256 / 2);
    for (RunLength = InitRL, PrevSuccess = i = 0; i < 256; i++) 
    {
        MinContext->Stats[i].Symbol = i;      
        MinContext->Stats[i].Freq = 1;
        MinContext->Stats[i].Successor = NULL;
    }
    for (i = 0; i < 128; i++)
        for (k = 0; k < 8; k++)
            for ( m=0; m < 64; m += 8)
                BinSumm[i][k + m] = BIN_SCALE - InitBinEsc[k] / (i + 2);
    for (i = 0; i < 25; i++)
        for (k = 0; k < 16; k++)            
            SEE2Cont[i][k].init(5*i+10);
  }

  void _FASTCALL StartModelRare(int MaxOrder)
  {
    int i, k, m ,Step;
    EscCount=PrintCount=1;
    if (MaxOrder < 2) 
    {
        memset(CharMask,0,sizeof(CharMask));
        OrderFall = this->MaxOrder;               
        MinContext = MaxContext;
        while (MinContext->Suffix != NULL) 
        {
            MinContext = MinContext->Suffix;  
            OrderFall--;
        }
        FoundState = MinContext->Stats;       
        MinContext = MaxContext;
    } 
    else 
    {
        this->MaxOrder = MaxOrder;                
        RestartModelRare();
        NS2BSIndx[0] = 2 * 0;                   
        NS2BSIndx[1] = 2 * 1;
        memset(NS2BSIndx + 2, 2 * 2, 9);          
        memset(NS2BSIndx + 11, 2 * 3, 256 - 11);
        for (i = 0; i < 3; i++)                 
          NS2Indx[i] = i;
        for (m = i, k = Step = 1; i < 256; i++) 
        {
            NS2Indx[i] =m;
            if ( !--k ) 
            { 
              k = ++Step;       
              m++; 
            }
        }
        memset(HB2Flag, 0, 0x40);             
        memset(HB2Flag + 0x40, 0x08, 0x100 - 0x40);
        DummySEE2Cont.Shift = PERIOD_BITS;
    }
  }

  PPM_CONTEXT* CreateSuccessors(BOOL Skip, PPM_CONTEXT::STATE* p1)
  {
    // static UpState declaration bypasses IntelC bug
    // static PPM_CONTEXT::STATE UpState;
    PPM_CONTEXT::STATE UpState;

    PPM_CONTEXT* pc = MinContext, * UpBranch = FoundState->Successor;
    PPM_CONTEXT::STATE * p, * ps[MAX_O], ** pps = ps;
    if ( !Skip ) 
    {
        *pps++ = FoundState;
        if ( !pc->Suffix )                  
          goto NO_LOOP;
    }
    if ( p1 ) 
    {
        p = p1;                               
        pc = pc->Suffix;
        goto LOOP_ENTRY;
    }
    do 
    {
        pc = pc->Suffix;
        if (pc->NumStats != 1) 
        {
            if ((p = pc->Stats)->Symbol != FoundState->Symbol)
                do { p++; } while (p->Symbol != FoundState->Symbol);
        } 
        else                              
          p = &(pc->oneState());
LOOP_ENTRY:
        if (p->Successor != UpBranch) 
        {
            pc = p->Successor;                
            break;
        }
        *pps++ = p;
    } 
    while ( pc->Suffix );
NO_LOOP:
    if (pps == ps)                          
      return pc;
    UpState.Symbol = *(BYTE*) UpBranch;
    UpState.Successor = (PPM_CONTEXT*) (((BYTE*) UpBranch)+1);
    if (pc->NumStats != 1) 
    {
        if ((p = pc->Stats)->Symbol != UpState.Symbol)
                do { p++; } while (p->Symbol != UpState.Symbol);
        UINT cf = p->Freq-1;
        UINT s0 = pc->SummFreq - pc->NumStats - cf;
        UpState.Freq = 1 + ((2 * cf <= s0) ? (5 * cf > s0) : 
            ((2 * cf + 3 * s0 - 1) / (2 * s0)));
    } 
    else                                  
      UpState.Freq = pc->oneState().Freq;
    do 
    {
        pc = pc->createChild(m_SubAllocator, *--pps, UpState);
        if ( !pc )                          
          return NULL;
    } 
    while (pps != ps);
    return pc;
  }

  void UpdateModel()
  {
    PPM_CONTEXT::STATE fs = *FoundState, * p = NULL;
    PPM_CONTEXT* pc, * Successor;
    UINT ns1, ns, cf, sf, s0;
    if (fs.Freq < MAX_FREQ/4 && (pc=MinContext->Suffix) != NULL) 
    {
        if (pc->NumStats != 1) 
        {
            if ((p = pc->Stats)->Symbol != fs.Symbol) 
            {
                do { p++; } while (p->Symbol != fs.Symbol);
                if (p[0].Freq >= p[-1].Freq) 
                {
                    _PPMD_SWAP(p[0],p[-1]); 
                    p--;
                }
            }
            if (p->Freq < MAX_FREQ-9) 
            {
                p->Freq += 2;               
                pc->SummFreq += 2;
            }
        } 
        else 
        {
            p = &(pc->oneState());            
            p->Freq += (p->Freq < 32);
        }
    }
    if ( !OrderFall ) 
    {
        MinContext = MaxContext = FoundState->Successor = CreateSuccessors(TRUE, p);
        if ( !MinContext )                  
          goto RESTART_MODEL;
        return;
    }
    *m_SubAllocator.pText++ = fs.Symbol;                   
    Successor = (PPM_CONTEXT*) m_SubAllocator.pText;
    if (m_SubAllocator.pText >= m_SubAllocator.UnitsStart)                
      goto RESTART_MODEL;
    if ( fs.Successor ) 
    {
        if ((BYTE*) fs.Successor <= m_SubAllocator.pText &&
                (fs.Successor=CreateSuccessors(FALSE,p)) == NULL)
                        goto RESTART_MODEL;
        if ( !--OrderFall ) 
        {
            Successor = fs.Successor;         
            m_SubAllocator.pText -= (MaxContext != MinContext);
        }
    } 
    else 
    {
        FoundState->Successor = Successor;    
        fs.Successor = MinContext;
    }
    s0 = MinContext->SummFreq - (ns = MinContext->NumStats) - (fs.Freq - 1);
    for (pc = MaxContext; pc != MinContext; pc = pc->Suffix) 
    {
        if ((ns1 = pc->NumStats) != 1) 
        {
            if ((ns1 & 1) == 0) 
            {
                pc->Stats = (PPM_CONTEXT::STATE*) m_SubAllocator.ExpandUnits(pc->Stats, ns1 >> 1);
                if ( !pc->Stats )           
                  goto RESTART_MODEL;
            }
            pc->SummFreq += (2 * ns1 < ns) + 2 * ((4 * ns1 <= ns) &
                    (pc->SummFreq <= 8 * ns1));
        } 
        else 
        {
            p = (PPM_CONTEXT::STATE*) m_SubAllocator.AllocUnits(1);
            if ( !p )                       
              goto RESTART_MODEL;
            *p = pc->oneState();              
            pc->Stats = p;
            if (p->Freq < MAX_FREQ / 4 - 1)     
              p->Freq += p->Freq;
            else                            
              p->Freq  = MAX_FREQ - 4;
            pc->SummFreq = p->Freq + InitEsc + (ns > 3);
        }
        cf = 2 * fs.Freq * (pc->SummFreq+6);      
        sf = s0 + pc->SummFreq;
        if (cf < 6 * sf) 
        {
            cf = 1 + (cf > sf)+(cf >= 4 * sf);
            pc->SummFreq += 3;
        } 
        else 
        {
            cf = 4 + (cf >= 9 * sf) + (cf >= 12 * sf) + (cf >= 15 * sf);
            pc->SummFreq += cf;
        }
        p = pc->Stats + ns1;                    
        p->Successor = Successor;
        p->Symbol = fs.Symbol;              
        p->Freq = cf;
        pc->NumStats = ++ns1;
    }
    MaxContext = MinContext = fs.Successor;
    return;
RESTART_MODEL:
    RestartModelRare();
    EscCount = 0;               
    PrintCount = 0xFF;
  }

  void ClearMask()
  {
    EscCount = 1;                             
    memset(CharMask, 0, sizeof(CharMask));
    // if (++PrintCount == 0)                  
    //   PrintInfo(DecodedFile,EncodedFile);
  }

  void update1(PPM_CONTEXT::STATE* p)
  {
    (FoundState = p)->Freq += 4;              
    MinContext->SummFreq += 4;
    if (p[0].Freq > p[-1].Freq) 
    {
        _PPMD_SWAP(p[0],p[-1]);             
        FoundState = --p;
        if (p->Freq > MAX_FREQ)             
          rescale();
    }
  }


  void update2(PPM_CONTEXT::STATE* p)
  {
    (FoundState = p)->Freq += 4;
    MinContext->SummFreq += 4;
    if (p->Freq > MAX_FREQ)                 
      rescale();
    EscCount++;                             
    RunLength = InitRL;
  }
  
  SEE2_CONTEXT* makeEscFreq2(int Diff,  UINT32 &aScale)
  {
    SEE2_CONTEXT* psee2c;
    if (MinContext->NumStats != 256) 
    {
      psee2c = SEE2Cont[NS2Indx[Diff-1]] + (Diff < MinContext->Suffix->NumStats - MinContext->NumStats)+
        2 * (MinContext->SummFreq < 11 * MinContext->NumStats) + 4 * (NumMasked > Diff) + HiBitsFlag;
      aScale = psee2c->getMean();
    } 
    else 
    {
      psee2c = &DummySEE2Cont;              
      aScale = 1;
    }
    return psee2c;
  }



  void rescale()
  {
    int OldNS = MinContext->NumStats, i = MinContext->NumStats - 1, Adder, EscFreq;
    PPM_CONTEXT::STATE* p1, * p;
    for (p = FoundState; p != MinContext->Stats; p--)       
      _PPMD_SWAP(p[0], p[-1]);
    MinContext->Stats->Freq += 4;                       
    MinContext->SummFreq += 4;
    EscFreq = MinContext->SummFreq - p->Freq;               
    Adder = (OrderFall != 0);
    MinContext->SummFreq = (p->Freq = (p->Freq + Adder) >> 1);
    do 
    {
        EscFreq -= (++p)->Freq;
        MinContext->SummFreq += (p->Freq = (p->Freq + Adder) >> 1);
        if (p[0].Freq > p[-1].Freq) 
        {
            PPM_CONTEXT::STATE tmp = *(p1 = p);
            do { p1[0] = p1[-1]; } while (--p1 != MinContext->Stats && tmp.Freq > p1[-1].Freq);
            *p1 = tmp;
        }
    } 
    while ( --i );
    if (p->Freq == 0) 
    {
        do { i++; } while ((--p)->Freq == 0);
        EscFreq += i;
        if ((MinContext->NumStats -= i) == 1) 
        {
            PPM_CONTEXT::STATE tmp = *MinContext->Stats;
            do { tmp.Freq -= (tmp.Freq >> 1); EscFreq >>= 1; } while (EscFreq > 1);
            m_SubAllocator.FreeUnits(MinContext->Stats, (OldNS+1) >> 1);
            *(FoundState = &MinContext->oneState()) = tmp;  return;
        }
    }
    MinContext->SummFreq += (EscFreq -= (EscFreq >> 1));
    int n0 = (OldNS+1) >> 1, n1 = (MinContext->NumStats + 1) >> 1;
    if (n0 != n1)
      MinContext->Stats = 
          (PPM_CONTEXT::STATE*) m_SubAllocator.ShrinkUnits(MinContext->Stats, n0, n1);
    FoundState = MinContext->Stats;
  }

  void NextContext()
  {
    if (!OrderFall && (BYTE*) FoundState->Successor > m_SubAllocator.pText)
      MinContext = MaxContext = FoundState->Successor;
    else 
    {
      UpdateModel();
      if (EscCount == 0)              
        ClearMask();
    }
  }
};

// Tabulated escapes for exponential symbol distribution
const BYTE ExpEscape[16]={ 25,14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))

}}

#endif
