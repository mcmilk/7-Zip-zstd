// DeflateExtConst.h

#pragma once

#ifndef __DEFLATEEXTCONST_H
#define __DEFLATEEXTCONST_H

#include "Common/Types.h"

namespace NArchive {
namespace NCab {
namespace NMSZip {

  const UINT32 kDistTableSize = 30;
  const UINT32 kHistorySize = 0x8000;
  const UINT32 kNumLenCombinations = 256;

  const UINT32 kNumHuffmanBits = 15;

}}}

#endif
