// Archive/Tar/ItemInfoEx.h

#pragma once

#ifndef __ARCHIVE_TAR_ITEMINFOEX_H
#define __ARCHIVE_TAR_ITEMINFOEX_H

#include "Archive/Tar/ItemInfo.h"
#include "Archive/Tar/Header.h"

namespace NArchive {
namespace NTar {
  
class CItemInfoEx: public CItemInfo
{
public:
  UINT64 HeaderPosition;
  UINT64 LongLinkSize;
  UINT64 GetDataPosition() const { return HeaderPosition + LongLinkSize + NFileHeader::kRecordSize; };
  UINT64 GetFullSize() const { return LongLinkSize + NFileHeader::kRecordSize + Size; };
};

}}

#endif



