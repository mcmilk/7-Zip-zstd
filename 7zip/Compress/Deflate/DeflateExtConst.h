// DeflateExtConst.h

#ifndef __DEFLATE_EXTCONST_H
#define __DEFLATE_EXTCONST_H

#include "Common/Types.h"

namespace NCompress {
namespace NDeflate {

  // const UInt32 kDistTableSize = 30;
  const UInt32 kDistTableSize32 = 30;
  const UInt32 kDistTableSize64 = 32;

  const UInt32 kHistorySize32 = 0x8000;
  const UInt32 kHistorySize64 = 0x10000;
  const UInt32 kNumLenCombinations32 = 256;
  const UInt32 kNumLenCombinations64 = 255;
  // don't change kNumLenCombinations64. It must be less than 255.

  const UInt32 kNumHuffmanBits = 15;

}}

#endif
