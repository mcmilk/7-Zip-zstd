// Archive/Cab/LZXExtConst.h

#ifndef __ARCHIVE_CAB_LZXEXTCONST_H
#define __ARCHIVE_CAB_LZXEXTCONST_H

namespace NArchive {
namespace NCab {
namespace NLZX {

const UINT32 kNumRepDistances = 3;

const UINT32 kNumLenSlots = 8;
const UINT32 kMatchMinLen = 2;
const UINT32 kNumLenSymbols = 249;
const UINT32 kMatchMaxLen = kMatchMinLen + (kNumLenSlots - 1) + kNumLenSymbols - 1;

const BYTE kNumAlignBits = 3;
const UINT32 kAlignTableSize = 1 << kNumAlignBits;

const UINT32 kNumHuffmanBits = 16;

const kNumPosSlotSymbols = 50;
const kNumPosSlotLenSlotSymbols = kNumPosSlotSymbols * kNumLenSlots;

const kMaxTableSize = 256 + kNumPosSlotLenSlotSymbols;



}}}

#endif;