// Archive/Deb/ItemInfoEx.h

#pragma once

#ifndef __ARCHIVE_DEB_ITEMINFOEX_H
#define __ARCHIVE_DEB_ITEMINFOEX_H

#include "Archive/Deb/ItemInfo.h"
#include "Archive/Deb/Header.h"

namespace NArchive {
namespace NDeb {
  
class CItemInfoEx: public CItemInfo
{
public:
  UINT64 HeaderPosition;
  UINT64 GetDataPosition() const { return HeaderPosition + sizeof(NHeader::CHeader); };
  // UINT64 GetFullSize() const { return NFileHeader::kRecordSize + Size; };
};

}}

#endif



