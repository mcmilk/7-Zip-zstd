// Archive/Cab/Header.h

#ifndef __ARCHIVE_CAB_HEADER_H
#define __ARCHIVE_CAB_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NCab {
namespace NHeader{

namespace NArchive 
{
  extern UInt32 kSignature;
  namespace NFlags
  {
    const int kPrevCabinet = 0x0001;
    const int kNextCabinet = 0x0002;
    const int kReservePresent = 0x0004;
  }
}

namespace NCompressionMethodMajor
{
  const Byte kNone = 0;
  const Byte kMSZip = 1;
  const Byte kQuantum = 2;
  const Byte kLZX = 3;
}

const int kFileNameIsUTFAttributeMask = 0x80;

namespace NFolderIndex
{
  const int kContinuedFromPrev    = 0xFFFD;
  const int kContinuedToNext      = 0xFFFE;
  const int kContinuedPrevAndNext = 0xFFFF;
}

}}}

#endif
