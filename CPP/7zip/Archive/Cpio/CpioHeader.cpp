// Archive/cpio/Header.h

#include "StdAfx.h"

#include "CpioHeader.h"

namespace NArchive {
namespace NCpio {
namespace NFileHeader {

  namespace NMagic 
  {
    extern const char *kMagic1 = "070701";
    extern const char *kMagic2 = "070702";
    extern const char *kMagic3 = "070707";
    extern const char *kEndName = "TRAILER!!!";

    const Byte kMagicForRecord2[2] = { 0xC7, 0x71 };
    // unsigned short kMagicForRecord2BE = 0xC771;
  }

}}}

