// Archive/cpio/Header.h

#include "StdAfx.h"

#include "Archive/cpio/Header.h"

namespace NArchive {
namespace Ncpio {
namespace NFileHeader {

  namespace NMagic 
  {
    extern const char *kMagic1 = "070701";
    extern const char *kMagic2 = "070702";
    extern const char *kEndName = "TRAILER!!!";

    extern unsigned short kMagicForRecord2 = 0x71C7;
    extern unsigned short kMagicForRecord2BE = 0xC771;
  }

}}}

