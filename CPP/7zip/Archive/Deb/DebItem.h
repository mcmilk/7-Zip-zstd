// Archive/Deb/ItemInfo.h

#ifndef __ARCHIVE_DEB_ITEMINFO_H
#define __ARCHIVE_DEB_ITEMINFO_H

#include "Common/Types.h"
#include "Common/MyString.h"
#include "DebHeader.h"

namespace NArchive {
namespace NDeb {

class CItem
{
public:
  AString Name;
  UInt64 Size;
  UInt32 ModificationTime;
  UInt32 Mode;
};

class CItemEx: public CItem
{
public:
  UInt64 HeaderPosition;
  UInt64 GetDataPosition() const { return HeaderPosition + NHeader::kHeaderSize; };
  // UInt64 GetFullSize() const { return NFileHeader::kRecordSize + Size; };
};

}}

#endif
