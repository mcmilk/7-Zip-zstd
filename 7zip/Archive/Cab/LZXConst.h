// Archive/Cab/LZXConst.h

#ifndef __ARCHIVE_CAB_LZXCONST_H
#define __ARCHIVE_CAB_LZXCONST_H

#include "LZXExtConst.h"

namespace NArchive {
namespace NCab {
namespace NLZX {


namespace NBlockType
{
  const int kNumBits = 3;
  enum EEnum
  {
    kVerbatim = 1,
    kAligned = 2,
    kUncompressed = 3
  };
}

const int kUncompressedBlockSizeNumBits = 24;

const UINT32 kLevelTableSize = 20;

const UINT32 kNumBitsForPreTreeLevel = 4;

const int kLevelSymbolZeros = 17;
const int kLevelSymbolZerosBig = 18;
const int kLevelSymbolSame = 19;

const int kLevelSymbolZerosStartValue = 4;
const int kLevelSymbolZerosNumBits = 4;

const int kLevelSymbolZerosBigStartValue = kLevelSymbolZerosStartValue + 
  (1 << kLevelSymbolZerosNumBits);
const int kLevelSymbolZerosBigNumBits = 5;

const int kNumBitsForAlignLevel = 3;

const int kLevelSymbolSameNumBits = 1;
const int kLevelSymbolSameStartValue = 4;
  
// const UINT32 kMainTableSize = 256 + kNumPosLenSlots + 1;

/*
const UINT32 kLenTableSize = 28;

const UINT32 kLenTableStart = kMainTableSize;
const UINT32 kAlignTableStart = kLenTableStart + kLenTableSize;

const UINT32 kHeapTablesSizesSum = kMainTableSize + kLenTableSize + kAlignTableSize;


const UINT32 kMaxTableSize = kHeapTablesSizesSum;

const UINT32 kTableDirectLevels = 16;
const UINT32 kTableLevelRepNumber = kTableDirectLevels;
const UINT32 kTableLevel0Number = kTableLevelRepNumber + 1;
const UINT32 kTableLevel0Number2 = kTableLevel0Number + 1;

const UINT32 kLevelMask = 0xF;

const UINT32 kPosLenNumber = 256;
const UINT32 kReadTableNumber = 256 + kNumPosLenSlots;

//const UINT32 kMatchNumber = kReadTableNumber + 1;

const BYTE kLenStart[kLenTableSize]      = 
  {0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224};
const BYTE kLenDirectBits[kLenTableSize] = 
  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};
*/

const UINT32 kDistStart[]  = 
{ 0,1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512,768,1024,
  1536,2048,3072,4096,6144,8192,12288,16384,24576,32768,49152,65536,98304,131072,196608,
  0x40000,
  0x60000,
  0x80000,
  0xA0000,
  0xC0000,
  0xE0000,

  0x100000, 
  0x120000, 
  0x140000, 
  0x160000, 
  0x180000, 
  0x1A0000, 
  0x1C0000, 
  0x1E0000
};
const BYTE kDistDirectBits[] = 
{
  0,0,0,0,1,1,2, 2, 3, 3, 4, 4, 5, 5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,16,16,
  17, 17, 17, 17, 17, 17, 
  17, 17,17, 17, 17, 17, 17, 17
};

/*
const BYTE kLevelDirectBits[kLevelTableSize] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 7};

const UINT32 kDistLimit2 = 0x101 - 1;
*/


}}}

#endif