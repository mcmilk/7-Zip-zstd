// Archive/Cab/LZXExtConst.h

#ifndef __ARCHIVE_CAB_LZXEXTCONST_H
#define __ARCHIVE_CAB_LZXEXTCONST_H

namespace NArchive {
namespace NCab {
namespace NLZX {

const UInt32 kNumRepDistances = 3;

const UInt32 kNumLenSlots = 8;
const UInt32 kMatchMinLen = 2;
const UInt32 kNumLenSymbols = 249;
const UInt32 kMatchMaxLen = kMatchMinLen + (kNumLenSlots - 1) + kNumLenSymbols - 1;

const Byte kNumAlignBits = 3;
const UInt32 kAlignTableSize = 1 << kNumAlignBits;

const UInt32 kNumHuffmanBits = 16;

const int kNumPosSlotSymbols = 50;
const int kNumPosSlotLenSlotSymbols = kNumPosSlotSymbols * kNumLenSlots;

const int kMaxTableSize = 256 + kNumPosSlotLenSlotSymbols;


}}}

#endif
