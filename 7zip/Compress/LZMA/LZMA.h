// LZMA.h

// #pragma once 

#include "LZMALen.h"

#ifndef __LZMA_H
#define __LZMA_H

namespace NCompress {
namespace NLZMA {

const UINT32 kNumRepDistances = 4;

const BYTE kNumStates = 12;

const BYTE kLiteralNextStates[kNumStates] = {0, 0, 0, 0, 1, 2, 3, 4,  5,  6,   4, 5};
const BYTE kMatchNextStates[kNumStates]   = {7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10};
const BYTE kRepNextStates[kNumStates]     = {8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11};
const BYTE kShortRepNextStates[kNumStates]= {9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11};

class CState
{
public:
  BYTE Index;
  void Init()
    { Index = 0; }
  void UpdateChar()
    { Index = kLiteralNextStates[Index]; }
  void UpdateMatch()
    { Index = kMatchNextStates[Index]; }
  void UpdateRep()
    { Index = kRepNextStates[Index]; }
  void UpdateShortRep()
    { Index = kShortRepNextStates[Index]; }
};

class CBaseCoder
{
protected:
  CState _state;
  BYTE _previousByte;
  bool _peviousIsMatch;
  UINT32 _repDistances[kNumRepDistances];
  void Init()
  {
    _state.Init();
    _previousByte = 0;
    _peviousIsMatch = false;
    for(int i = 0 ; i < kNumRepDistances; i++)
      _repDistances[i] = 0;
  }
};

const int kNumPosSlotBits = 6; 
const int kDicLogSizeMin = 0; 
const int kDicLogSizeMax = 32; 
const int kDistTableSizeMax = kDicLogSizeMax * 2; 

const UINT32 kNumLenToPosStates = 4;
inline UINT32 GetLenToPosState(UINT32 len)
{
  len -= 2;
  if (len < kNumLenToPosStates)
    return len;
  return kNumLenToPosStates - 1;
}

const UINT32 kMatchMinLen = 2;

const UINT32 kMatchMaxLen = kMatchMinLen + NLength::kNumSymbolsTotal - 1;

const int kNumAlignBits = 4;
const UINT32 kAlignTableSize = 1 << kNumAlignBits;
const UINT32 kAlignMask = (kAlignTableSize - 1);

const UINT32 kStartPosModelIndex = 4;
const UINT32 kEndPosModelIndex = 14;
const UINT32 kNumPosModels = kEndPosModelIndex - kStartPosModelIndex;

const UINT32 kNumFullDistances = 1 << (kEndPosModelIndex / 2);

const UINT32 kMainChoiceLiteralIndex = 0;
const UINT32 kMainChoiceMatchIndex = 1;

const UINT32 kMatchChoiceDistanceIndex= 0;
const UINT32 kMatchChoiceRepetitionIndex = 1;

const int kNumMoveBits = 5;

const int kNumLitPosStatesBitsEncodingMax = 4;
const int kNumLitContextBitsMax = 8;

}}

#endif
