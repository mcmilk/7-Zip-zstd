// DeflateExtConst.h

#pragma once

#ifndef __DEFLATE_EXTCONST_H
#define __DEFLATE_EXTCONST_H

#include "Common/Types.h"

namespace NCompress {
namespace NDeflate {

  // const UINT32 kDistTableSize = 30;
  const UINT32 kDistTableSize32 = 30;
  const UINT32 kDistTableSize64 = 32;

  const UINT32 kHistorySize32 = 0x8000;
  const UINT32 kHistorySize64 = 0x10000;
  const UINT32 kNumLenCombinations32 = 256;
  const UINT32 kNumLenCombinations64 = 255;
  // don't change kNumLenCombinations64. It must be less than 255.

  const UINT32 kNumHuffmanBits = 15;

}}

#endif
