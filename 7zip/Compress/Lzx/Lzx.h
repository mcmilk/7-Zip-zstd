// Lzx.h

#ifndef __COMPRESS_LZX_H
#define __COMPRESS_LZX_H

namespace NCompress {
namespace NLzx {

const int kNumHuffmanBits = 16;
const UInt32 kNumRepDistances = 3;

const UInt32 kNumLenSlots = 8;
const UInt32 kMatchMinLen = 2;
const UInt32 kNumLenSymbols = 249;
const UInt32 kMatchMaxLen = kMatchMinLen + (kNumLenSlots - 1) + kNumLenSymbols - 1;

const int kNumAlignBits = 3;
const UInt32 kAlignTableSize = 1 << kNumAlignBits;

const UInt32 kNumPosSlots = 50;
const UInt32 kNumPosLenSlots = kNumPosSlots * kNumLenSlots;

const UInt32 kMainTableSize = 256 + kNumPosLenSlots;
const UInt32 kLevelTableSize = 20;
const UInt32 kMaxTableSize = kMainTableSize;

const int kNumBlockTypeBits = 3;
const int kBlockTypeVerbatim = 1;
const int kBlockTypeAligned = 2;
const int kBlockTypeUncompressed = 3;

const int kUncompressedBlockSizeNumBits = 24;

const int kNumBitsForPreTreeLevel = 4;

const int kLevelSymbolZeros = 17;
const int kLevelSymbolZerosBig = 18;
const int kLevelSymbolSame = 19;

const int kLevelSymbolZerosStartValue = 4;
const int kLevelSymbolZerosNumBits = 4;

const int kLevelSymbolZerosBigStartValue = kLevelSymbolZerosStartValue + 
    (1 << kLevelSymbolZerosNumBits);
const int kLevelSymbolZerosBigNumBits = 5;

const int kLevelSymbolSameNumBits = 1;
const int kLevelSymbolSameStartValue = 4;

const int kNumBitsForAlignLevel = 3;
  
const int kNumDictionaryBitsMin = 15;
const int kNumDictionaryBitsMax = 21;
const UInt32 kDictionarySizeMax = (1 << kNumDictionaryBitsMax);

const int kNumLinearPosSlotBits = 17;
const UInt32 kNumPowerPosSlots = 0x26;

}}

#endif
