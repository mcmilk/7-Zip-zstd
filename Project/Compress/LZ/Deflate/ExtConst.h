// Deflate/ExtConst.h

#pragma once

#ifndef __DEFLATE_EXTCONST_H
#define __DEFLATE_EXTCONST_H

#include "Common/Types.h"

namespace NDeflate {

  const UINT32 kDistTableSize = 30;
  const UINT32 kHistorySize = 0x8000;
  const UINT32 kNumLenCombinations = 256;

  const UINT32 kNumHuffmanBits = 15;

}

#endif
