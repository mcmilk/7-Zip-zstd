// Deflate/Const.h

#pragma once

#ifndef __DEFLATECONST_H
#define __DEFLATECONST_H

#include "DeflateExtConst.h"

namespace NDeflate {

const UINT32 kLenTableSize = 29;

const UINT32 kStaticDistTableSize = 32;
const UINT32 kStaticLenTableSize = 31;

const UINT32 kReadTableNumber = 0x100;
const UINT32 kMatchNumber = kReadTableNumber + 1;

const UINT32 kMainTableSize = kMatchNumber + kLenTableSize; //298;
const UINT32 kStaticMainTableSize = kMatchNumber + kStaticLenTableSize; //298;

const UINT32 kDistTableStart = kMainTableSize;

const UINT32 kHeapTablesSizesSum = kMainTableSize + kDistTableSize;

const UINT32 kLevelTableSize = 19;

const UINT32 kMaxTableSize = kHeapTablesSizesSum; // test it
const UINT32 kStaticMaxTableSize = kStaticMainTableSize + kStaticDistTableSize;

const UINT32 kTableDirectLevels = 16;
const UINT32 kTableLevelRepNumber = kTableDirectLevels;
const UINT32 kTableLevel0Number = kTableLevelRepNumber + 1;
const UINT32 kTableLevel0Number2 = kTableLevel0Number + 1;

const UINT32 kLevelMask = 0xF;

const BYTE kLenStart[kLenTableSize]      = {0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224, 255};
const BYTE kLenDirectBits[kLenTableSize] = {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5, 0};


const UINT32 kDistStart[kDistTableSize]     = {0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096,6144,8192,12288,16384,24576};
const BYTE kDistDirectBits[kDistTableSize] = {0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,  6,  6,  7,  7,  8,  8,   9,   9,  10,  10,  11,  11,  12,   12,   13,   13};

const BYTE kLevelDirectBits[kLevelTableSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7};

const BYTE kCodeLengthAlphabetOrder[kLevelTableSize] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

const UINT32 kMatchMinLen = 3; 
const UINT32 kMatchMaxLen = kNumLenCombinations + kMatchMinLen - 1; //255 + 2; test it

const kFinalBlockFieldSize = 1;

namespace NFinalBlockField
{
enum
{
  kNotFinalBlock = 0,
  kFinalBlock = 1
};
}

const kBlockTypeFieldSize = 2;

namespace NBlockType
{
  enum
  {
    kStored = 0,
    kFixedHuffman = 1,
    kDynamicHuffman = 2,
    kReserved = 3
  };
}

const UINT32 kDeflateNumberOfLengthCodesFieldSize  = 5;
const UINT32 kDeflateNumberOfDistanceCodesFieldSize  = 5;
const UINT32 kDeflateNumberOfLevelCodesFieldSize  = 4;

const UINT32 kDeflateNumberOfLitLenCodesMin = 257;

const UINT32 kDeflateNumberOfDistanceCodesMin = 1;
const UINT32 kDeflateNumberOfLevelCodesMin = 4;

const UINT32 kDeflateLevelCodeFieldSize  = 3;

const UINT32 kDeflateStoredBlockLengthFieldSizeSize = 16;

}

#endif