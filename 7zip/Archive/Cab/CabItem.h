// Archive/Cab/ItemInfo.h

#pragma once

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
  UINT16 Flags;
  UINT64 UnPackSize;
  UINT32 UnPackOffset;
  UINT16 FolderIndex;
  UINT32 Time;
  UINT16  Attributes;
  UINT32 GetWinAttributes() const { return Attributes & (Attributes & ~NHeader::kFileNameIsUTFAttributeMask); }
  bool IsNameUTF() const { return (Attributes & NHeader::kFileNameIsUTFAttributeMask) != 0; }
  AString Name;
};

}}

#endif


