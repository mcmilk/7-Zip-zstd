// Archive/Cab/ItemInfo.h

#ifndef __ARCHIVE_RAR_ITEMINFO_H
#define __ARCHIVE_RAR_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"
#include "CabHeader.h"

namespace NArchive {
namespace NCab {

class CItem
{
public:
  UInt16 Flags;
  UInt64 UnPackSize;
  UInt32 UnPackOffset;
  UInt16 FolderIndex;
  UInt32 Time;
  UInt16  Attributes;
  UInt32 GetWinAttributes() const { return Attributes & (Attributes & ~NHeader::kFileNameIsUTFAttributeMask); }
  bool IsNameUTF() const { return (Attributes & NHeader::kFileNameIsUTFAttributeMask) != 0; }
  bool IsDirectory() const { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0; }
  AString Name;
};

}}

#endif


