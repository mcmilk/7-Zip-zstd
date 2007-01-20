// Archive/IsoHeader.h

#ifndef __ARCHIVE_ISO_HEADER_H
#define __ARCHIVE_ISO_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NIso {

namespace NVolDescType
{
  const Byte kBootRecord = 0;
  const Byte kPrimaryVol = 1;
  const Byte kSupplementaryVol = 2;
  const Byte kVolParttition = 3;
  const Byte kTerminator = 255;
}

const Byte kVersion = 1;

namespace NFileFlags
{
  const Byte kDirectory = 1 << 1;
}

extern const char *kElToritoSpec;

const UInt32 kStartPos = 0x8000;

namespace NBootEntryId
{
  const Byte kValidationEntry = 1;
  const Byte kInitialEntryNotBootable = 0;
  const Byte kInitialEntryBootable = 0x88;
}

namespace NBootPlatformId
{
  const Byte kX86 = 0;
  const Byte kPowerPC = 1;
  const Byte kMac = 2;
}

const BYTE kBootMediaTypeMask = 0xF;

namespace NBootMediaType
{
  const Byte kNoEmulation = 0;
  const Byte k1d2Floppy = 1;
  const Byte k1d44Floppy = 2;
  const Byte k2d88Floppy = 3;
  const Byte kHardDisk = 4;
}

const int kNumBootMediaTypes = 5;
extern const wchar_t *kMediaTypes[];

}}

#endif
