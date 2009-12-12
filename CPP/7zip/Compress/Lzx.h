// Lzx.h

#ifndef __COMPRESS_LZX_H
#define __COMPRESS_LZX_H

namespace NCompress {
namespace NLzx {

const unsigned kNumHuffmanBits = 16;
const UInt32 kNumRepDistances = 3;

const UInt32 kNumLenSlots = 8;
const UInt32 kMatchMinLen = 2;
const UInt32 kNumLenSymbols = 249;
const UInt32 kMatchMaxLen = kMatchMinLen + (kNumLenSlots - 1) + kNumLenSymbols - 1;

const unsigned kNumAlignBits = 3;
const UInt32 kAlignTableSize = 1 << kNumAlignBits;

const UInt32 kNumPosSlots = 50;
const UInt32 kNumPosLenSlots = kNumPosSlots * kNumLenSlots;

const UInt32 kMainTableSize = 256 + kNumPosLenSlots;
const UInt32 kLevelTableSize = 20;
const UInt32 kMaxTableSize = kMainTableSize;

const unsigned kNumBlockTypeBits = 3;
const unsigned kBlockTypeVerbatim = 1;
const unsigned kBlockTypeAligned = 2;
const unsigned kBlockTypeUncompressed = 3;

const unsigned kUncompressedBlockSizeNumBits = 24;

const unsigned kNumBitsForPreTreeLevel = 4;

const unsigned kLevelSymbolZeros = 17;
const unsigned kLevelSymbolZerosBig = 18;
const unsigned kLevelSymbolSame = 19;

const unsigned kLevelSymbolZerosStartValue = 4;
const unsigned kLevelSymbolZerosNumBits = 4;

const unsigned kLevelSymbolZerosBigStartValue = kLevelSymbolZerosStartValue +
    (1 << kLevelSymbolZerosNumBits);
const unsigned kLevelSymbolZerosBigNumBits = 5;

const unsigned kLevelSymbolSameNumBits = 1;
const unsigned kLevelSymbolSameStartValue = 4;

const unsigned kNumBitsForAlignLevel = 3;
  
const unsigned kNumDictionaryBitsMin = 15;
const unsigned kNumDictionaryBitsMax = 21;
const UInt32 kDictionarySizeMax = (1 << kNumDictionaryBitsMax);

const unsigned kNumLinearPosSlotBits = 17;
const UInt32 kNumPowerPosSlots = 0x26;

}}

#endif
