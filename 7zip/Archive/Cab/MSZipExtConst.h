// DeflateExtConst.h

#ifndef __DEFLATEEXTCONST_H
#define __DEFLATEEXTCONST_H

#include "Common/Types.h"

namespace NArchive {
namespace NCab {
namespace NMSZip {

  const UInt32 kDistTableSize = 30;
  const UInt32 kHistorySize = 0x8000;
  const UInt32 kNumLenCombinations = 256;

  const UInt32 kNumHuffmanBits = 15;

}}}

#endif
