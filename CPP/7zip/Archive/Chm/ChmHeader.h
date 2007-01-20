// Archive/Chm/Header.h

#ifndef __ARCHIVE_CHM_HEADER_H
#define __ARCHIVE_CHM_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NChm {
namespace NHeader{

const UInt32 kItspSignature = 0x50535449;
const UInt32 kPmglSignature = 0x4C474D50;
const UInt32 kLzxcSignature = 0x43585A4C;

const UInt32 kIfcmSignature = 0x4D434649;
const UInt32 kAollSignature = 0x4C4C4F41;
const UInt32 kCaolSignature = 0x4C4F4143;

extern UInt32 kItsfSignature;

extern UInt32 kItolSignature;
const UInt32 kItlsSignature = 0x534C5449;
UInt64 inline GetHxsSignature() { return ((UInt64)kItlsSignature << 32) | kItolSignature; }
  
}}}

#endif
