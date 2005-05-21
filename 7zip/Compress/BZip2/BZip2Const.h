// Compress/BZip2Const.h

#ifndef __COMPRESS_BZIP2_CONST_H
#define __COMPRESS_BZIP2_CONST_H

namespace NCompress {
namespace NBZip2 {

const Byte kArSig0 = 'B';
const Byte kArSig1 = 'Z';
const Byte kArSig2 = 'h';
const Byte kArSig3 = '0';

const Byte kFinSig0 = 0x17;
const Byte kFinSig1 = 0x72;
const Byte kFinSig2 = 0x45;
const Byte kFinSig3 = 0x38;
const Byte kFinSig4 = 0x50;
const Byte kFinSig5 = 0x90;

const Byte kBlockSig0 = 0x31;
const Byte kBlockSig1 = 0x41;
const Byte kBlockSig2 = 0x59;
const Byte kBlockSig3 = 0x26;
const Byte kBlockSig4 = 0x53;
const Byte kBlockSig5 = 0x59;

const int kNumOrigBits = 24;

const int kNumTablesBits = 3;
const int kNumTablesMin = 2;
const int kNumTablesMax = 6;

const int kNumLevelsBits = 5;

const int kMaxHuffmanLen = 20; // Check it

const int kMaxAlphaSize = 258;

const int kGroupSize = 50;

const int kBlockSizeMultMin = 1;
const int kBlockSizeMultMax = 9;
const UInt32 kBlockSizeStep = 100000;
const UInt32 kBlockSizeMax = kBlockSizeMultMax * kBlockSizeStep;

const int kNumSelectorsBits = 15;
const UInt32 kNumSelectorsMax = (2 + (kBlockSizeMax / kGroupSize));

const int kRleModeRepSize = 4;

}}

#endif
