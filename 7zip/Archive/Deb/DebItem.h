// Archive/Deb/ItemInfo.h

#pragma once

#ifndef __ARCHIVE_DEB_ITEMINFO_H
#define __ARCHIVE_DEB_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"
#include "DebHeader.h"

namespace NArchive {
namespace NDeb {

class CItem
{
public:
  AString Name;
  UINT64 Size;
  UINT32 ModificationTime;
  UINT32 Mode;
};

class CItemEx: public CItem
{
public:
  UINT64 HeaderPosition;
  UINT64 GetDataPosition() const { return HeaderPosition + sizeof(NHeader::CHeader); };
  // UINT64 GetFullSize() const { return NFileHeader::kRecordSize + Size; };
};

}}

#endif
