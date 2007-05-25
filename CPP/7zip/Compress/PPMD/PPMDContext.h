// Compress/PPMD/Context.h
// This code is based on Dmitry Shkarin's PPMdH code

#ifndef __COMPRESS_PPMD_CONTEXT_H
#define __COMPRESS_PPMD_CONTEXT_H

#include "../../../Common/Types.h"

#include "../RangeCoder/RangeCoder.h"
#include "PPMDSubAlloc.h"

namespace NCompress {
namespace NPPMD {

const int INT_BITS=7, PERIOD_BITS=7, TOT_BITS=INT_BITS+PERIOD_BITS,
        INTERVAL=1 << INT_BITS, BIN_SCALE=1 << TOT_BITS, MAX_FREQ=124;

struct SEE2_CONTEXT 
{   
  // SEE-contexts for PPM-contexts with masked symbols
  UInt16 Summ;
  Byte Shift, Count;
  void init(int InitVal) { Summ = (UInt16)(InitVal << (Shift=PERIOD_BITS-4)); Count=4; }
  unsigned int getMean() 
  {
    unsigned int RetVal=(Summ >> Shift);        
    Summ = (UInt16)(Summ - RetVal);
    return RetVal+(RetVal == 0);
  }
  void update() 
  {
    if (Shift < PERIOD_BITS && --Count == 0) 
    {
      Summ <<= 1;                   
      Count = (Byte)(3 << Shift++);
    }
  }
};

struct PPM_CONTEXT 
{
  UInt16 NumStats; // sizeof(UInt16) > sizeof(Byte) 
  UInt16 SummFreq;
  
  struct STATE 
  { 
    Byte Symbol, Freq; 
    UInt16 SuccessorLow;
    UInt16 SuccessorHigh;

    UInt32 GetSuccessor() const { return SuccessorLow | ((UInt32)SuccessorHigh << 16); }
    void SetSuccessor(UInt32 v) 
    { 
      SuccessorLow = (UInt16)(v & 0xFFFF);
      SuccessorHigh = (UInt16)((v >> 16) & 0xFFFF);
    }
  };
  
  UInt32 Stats;
  UInt32 Suffix;
  
  PPM_CONTEXT* createChild(CSubAllocator &subAllocator, STATE* pStats, STATE& FirstState)
  {
    PPM_CONTEXT* pc = (PPM_CONTEXT*) subAllocator.AllocContext();
    if (pc) 
    {
      pc->NumStats = 1;                     
      pc->oneState() = FirstState;
      pc->Suffix = subAllocator.GetOffset(this);                    
      pStats->SetSuccessor(subAllocator.GetOffsetNoCheck(pc));
    }
    return pc;
  }

  STATE& oneState() const { return (STATE&) SummFreq; }
};

/////////////////////////////////

const UInt16 InitBinEsc[] =
  {0x3CDD, 0x1F3F, 0x59BF, 0x48F3, 0x64A1, 0x5ABC, 0x6632, 0x6051};

struct CInfo
{
  CSubAllocator SubAllocator;
  SEE2_CONTEXT SEE2Cont[25][16], DummySEE2Cont;
  PPM_CONTEXT * MinContext, * MaxContext;

  PPM_CONTEXT::STATE* FoundState;      // found next state transition
  int NumMasked, InitEsc, OrderFall, RunLength, InitRL, MaxOrder;
  Byte CharMask[256], NS2Indx[256], NS2BSIndx[256], HB2Flag[256];
  Byte EscCount, PrintCount, PrevSuccess, HiBitsFlag;
  UInt16 BinSumm[128][64];               // binary SEE-contexts

  UInt16 &GetBinSumm(const PPM_CONTEXT::STATE &rs, int numStates)
  {
    HiBitsFlag = HB2Flag[FoundState->Symbol];
    return BinSumm[rs.Freq - 1][
         PrevSuccess + NS2BSIndx[numStates - 1] +
         HiBitsFlag + 2 * HB2Flag[rs.Symbol] + 
         ((RunLength >> 26) & 0x20)];
  }

  PPM_CONTEXT *GetContext(UInt32 offset) const { return (PPM_CONTEXT *)SubAllocator.GetPtr(offset); }
  PPM_CONTEXT *GetContextNoCheck(UInt32 offset) const { return (PPM_CONTEXT *)SubAllocator.GetPtrNoCheck(offset); }
  PPM_CONTEXT::STATE *GetState(UInt32 offset) const { return (PPM_CONTEXT::STATE *)SubAllocator.GetPtr(offset); }
  PPM_CONTEXT::STATE *GetStateNoCheck(UInt32 offset) const { return (PPM_CONTEXT::STATE *)SubAllocator.GetPtr(offset); }

  void RestartModelRare()
  {
    int i, k, m;
    memset(CharMask,0,sizeof(CharMask));
    SubAllocator.InitSubAllocator();                     
    InitRL = -((MaxOrder < 12) ? MaxOrder : 12) - 1;
    MinContext = MaxContext = (PPM_CONTEXT*) SubAllocator.AllocContext();
    MinContext->Suffix = 0;                
    OrderFall = MaxOrder;
    MinContext->SummFreq = (UInt16)((MinContext->NumStats = 256) + 1);
    FoundState = (PPM_CONTEXT::STATE*)SubAllocator.AllocUnits(256 / 2);
    MinContext->Stats = SubAllocator.GetOffsetNoCheck(FoundState);
    PrevSuccess = 0;
    for (RunLength = InitRL, i = 0; i < 256; i++) 
    {
      PPM_CONTEXT::STATE &state = FoundState[i];
      state.Symbol = (Byte)i;      
      state.Freq = 1;
      state.SetSuccessor(0);
    }
    for (i = 0; i < 128; i++)
        for (k = 0; k < 8; k++)
            for ( m=0; m < 64; m += 8)
                BinSumm[i][k + m] = (UInt16)(BIN_SCALE - InitBinEsc[k] / (i + 2));
    for (i = 0; i < 25; i++)
        for (k = 0; k < 16; k++)            
            SEE2Cont[i][k].init(5*i+10);
  }

  void StartModelRare(int maxOrder)
  {
    int i, k, m ,Step;
    EscCount=PrintCount=1;
    if (maxOrder < 2) 
    {
        memset(CharMask,0,sizeof(CharMask));
        OrderFall = MaxOrder;               
        MinContext = MaxContext;
        while (MinContext->Suffix != 0) 
        {
          MinContext = GetContextNoCheck(MinContext->Suffix);  
          OrderFall--;
        }
        FoundState = GetState(MinContext->Stats);
        MinContext = MaxContext;
    } 
    else 
    {
        MaxOrder = maxOrder;                
        RestartModelRare();
        NS2BSIndx[0] = 2 * 0;                   
        NS2BSIndx[1] = 2 * 1;
        memset(NS2BSIndx + 2, 2 * 2, 9);          
        memset(NS2BSIndx + 11, 2 * 3, 256 - 11);
        for (i = 0; i < 3; i++)                 
          NS2Indx[i] = (Byte)i;
        for (m = i, k = Step = 1; i < 256; i++) 
        {
            NS2Indx[i] = (Byte)m;
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

  PPM_CONTEXT* CreateSuccessors(bool skip, PPM_CONTEXT::STATE* p1)
  {
    // static UpState declaration bypasses IntelC bug
    // static PPM_CONTEXT::STATE UpState;
    PPM_CONTEXT::STATE UpState;

    PPM_CONTEXT *pc = MinContext;
    PPM_CONTEXT *UpBranch = GetContext(FoundState->GetSuccessor());
    PPM_CONTEXT::STATE * p, * ps[MAX_O], ** pps = ps;
    if ( !skip ) 
    {
        *pps++ = FoundState;
        if ( !pc->Suffix )                  
          goto NO_LOOP;
    }
    if ( p1 ) 
    {
        p = p1;                               
        pc = GetContext(pc->Suffix);
        goto LOOP_ENTRY;
    }
    do 
    {
        pc = GetContext(pc->Suffix);
        if (pc->NumStats != 1) 
        {
            if ((p = GetStateNoCheck(pc->Stats))->Symbol != FoundState->Symbol)
                do { p++; } while (p->Symbol != FoundState->Symbol);
        } 
        else                              
          p = &(pc->oneState());
LOOP_ENTRY:
        if (GetContext(p->GetSuccessor()) != UpBranch) 
        {
            pc = GetContext(p->GetSuccessor());                
            break;
        }
        *pps++ = p;
    } 
    while ( pc->Suffix );
NO_LOOP:
    if (pps == ps)                          
      return pc;
    UpState.Symbol = *(Byte*) UpBranch;
    UpState.SetSuccessor(SubAllocator.GetOffset(UpBranch) + 1);
    if (pc->NumStats != 1) 
    {
        if ((p = GetStateNoCheck(pc->Stats))->Symbol != UpState.Symbol)
                do { p++; } while (p->Symbol != UpState.Symbol);
        unsigned int cf = p->Freq-1;
        unsigned int s0 = pc->SummFreq - pc->NumStats - cf;
        UpState.Freq = (Byte)(1 + ((2 * cf <= s0) ? (5 * cf > s0) : 
            ((2 * cf + 3 * s0 - 1) / (2 * s0))));
    } 
    else                                  
      UpState.Freq = pc->oneState().Freq;
    do 
    {
        pc = pc->createChild(SubAllocator, *--pps, UpState);
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
    unsigned int ns1, ns, cf, sf, s0;
    if (fs.Freq < MAX_FREQ / 4 && MinContext->Suffix != 0) 
    {
        pc = GetContextNoCheck(MinContext->Suffix);
      
        if (pc->NumStats != 1) 
        {
            if ((p = GetStateNoCheck(pc->Stats))->Symbol != fs.Symbol) 
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
            p->Freq = (Byte)(p->Freq + ((p->Freq < 32) ? 1 : 0));
        }
    }
    if ( !OrderFall ) 
    {
        MinContext = MaxContext = CreateSuccessors(true, p);
        FoundState->SetSuccessor(SubAllocator.GetOffset(MinContext));
        if (MinContext == 0)                  
          goto RESTART_MODEL;
        return;
    }
    *SubAllocator.pText++ = fs.Symbol;                   
    Successor = (PPM_CONTEXT*) SubAllocator.pText;
    if (SubAllocator.pText >= SubAllocator.UnitsStart)                
      goto RESTART_MODEL;
    if (fs.GetSuccessor() != 0) 
    {
        if ((Byte *)GetContext(fs.GetSuccessor()) <= SubAllocator.pText)
        {
          PPM_CONTEXT* cs = CreateSuccessors(false, p);
          fs.SetSuccessor(SubAllocator.GetOffset(cs));
          if (cs == NULL)
            goto RESTART_MODEL;
        }
        if ( !--OrderFall ) 
        {
            Successor = GetContext(fs.GetSuccessor());
            SubAllocator.pText -= (MaxContext != MinContext);
        }
    } 
    else 
    {
        FoundState->SetSuccessor(SubAllocator.GetOffsetNoCheck(Successor));    
        fs.SetSuccessor(SubAllocator.GetOffsetNoCheck(MinContext));
    }
    s0 = MinContext->SummFreq - (ns = MinContext->NumStats) - (fs.Freq - 1);
    for (pc = MaxContext; pc != MinContext; pc = GetContext(pc->Suffix)) 
    {
        if ((ns1 = pc->NumStats) != 1) 
        {
            if ((ns1 & 1) == 0) 
            {
                void *ppp = SubAllocator.ExpandUnits(GetState(pc->Stats), ns1 >> 1);
                pc->Stats = SubAllocator.GetOffset(ppp);
                if (!ppp)           
                  goto RESTART_MODEL;
            }
            pc->SummFreq = (UInt16)(pc->SummFreq + (2 * ns1 < ns) + 2 * ((4 * ns1 <= ns) &
                    (pc->SummFreq <= 8 * ns1)));
        } 
        else 
        {
            p = (PPM_CONTEXT::STATE*) SubAllocator.AllocUnits(1);
            if ( !p )                       
              goto RESTART_MODEL;
            *p = pc->oneState();              
            pc->Stats = SubAllocator.GetOffsetNoCheck(p);
            if (p->Freq < MAX_FREQ / 4 - 1)     
              p->Freq <<= 1;
            else                            
              p->Freq  = MAX_FREQ - 4;
            pc->SummFreq = (UInt16)(p->Freq + InitEsc + (ns > 3));
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
            pc->SummFreq = (UInt16)(pc->SummFreq + cf);
        }
        p = GetState(pc->Stats) + ns1;                    
        p->SetSuccessor(SubAllocator.GetOffset(Successor));
        p->Symbol = fs.Symbol;              
        p->Freq = (Byte)cf;
        pc->NumStats = (UInt16)++ns1;
    }
    MaxContext = MinContext = GetContext(fs.GetSuccessor());
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
  
  SEE2_CONTEXT* makeEscFreq2(int Diff, UInt32 &scale)
  {
    SEE2_CONTEXT* psee2c;
    if (MinContext->NumStats != 256) 
    {
      psee2c = SEE2Cont[NS2Indx[Diff-1]] + 
        (Diff < (GetContext(MinContext->Suffix))->NumStats - MinContext->NumStats) +
        2 * (MinContext->SummFreq < 11 * MinContext->NumStats) + 
        4 * (NumMasked > Diff) + 
        HiBitsFlag;
      scale = psee2c->getMean();
    } 
    else 
    {
      psee2c = &DummySEE2Cont;              
      scale = 1;
    }
    return psee2c;
  }



  void rescale()
  {
    int OldNS = MinContext->NumStats, i = MinContext->NumStats - 1, Adder, EscFreq;
    PPM_CONTEXT::STATE* p1, * p;
    PPM_CONTEXT::STATE *stats = GetStateNoCheck(MinContext->Stats);
    for (p = FoundState; p != stats; p--)       
      _PPMD_SWAP(p[0], p[-1]);
    stats->Freq += 4;                       
    MinContext->SummFreq += 4;
    EscFreq = MinContext->SummFreq - p->Freq;               
    Adder = (OrderFall != 0);
    p->Freq = (Byte)((p->Freq + Adder) >> 1);
    MinContext->SummFreq = p->Freq;
    do 
    {
        EscFreq -= (++p)->Freq;
        p->Freq = (Byte)((p->Freq + Adder) >> 1);
        MinContext->SummFreq = (UInt16)(MinContext->SummFreq + p->Freq);
        if (p[0].Freq > p[-1].Freq) 
        {
            PPM_CONTEXT::STATE tmp = *(p1 = p);
            do 
            { 
              p1[0] = p1[-1]; 
            } 
            while (--p1 != stats && tmp.Freq > p1[-1].Freq);
            *p1 = tmp;
        }
    } 
    while ( --i );
    if (p->Freq == 0) 
    {
        do { i++; } while ((--p)->Freq == 0);
        EscFreq += i;
        MinContext->NumStats = (UInt16)(MinContext->NumStats - i);
        if (MinContext->NumStats == 1) 
        {
            PPM_CONTEXT::STATE tmp = *stats;
            do { tmp.Freq = (Byte)(tmp.Freq - (tmp.Freq >> 1)); EscFreq >>= 1; } while (EscFreq > 1);
            SubAllocator.FreeUnits(stats, (OldNS+1) >> 1);
            *(FoundState = &MinContext->oneState()) = tmp;  return;
        }
    }
    EscFreq -= (EscFreq >> 1);
    MinContext->SummFreq = (UInt16)(MinContext->SummFreq + EscFreq);
    int n0 = (OldNS+1) >> 1, n1 = (MinContext->NumStats + 1) >> 1;
    if (n0 != n1)
      MinContext->Stats = SubAllocator.GetOffset(SubAllocator.ShrinkUnits(stats, n0, n1));
    FoundState = GetState(MinContext->Stats);
  }

  void NextContext()
  {
    PPM_CONTEXT *c = GetContext(FoundState->GetSuccessor());
    if (!OrderFall && (Byte *)c > SubAllocator.pText)
      MinContext = MaxContext = c;
    else 
    {
      UpdateModel();
      if (EscCount == 0)              
        ClearMask();
    }
  }
};

// Tabulated escapes for exponential symbol distribution
const Byte ExpEscape[16]={ 25,14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))

}}

#endif
