// TarItem.h

#ifndef __ARCHIVE_TAR_ITEM_H
#define __ARCHIVE_TAR_ITEM_H

#include "../Common/ItemNameUtils.h"

#include "TarHeader.h"

namespace NArchive {
namespace NTar {

struct CItem
{
  AString Name;
  UInt64 Size;

  UInt32 Mode;
  UInt32 UID;
  UInt32 GID;
  UInt32 MTime;
  UInt32 DeviceMajor;
  UInt32 DeviceMinor;

  AString LinkName;
  AString User;
  AString Group;

  char Magic[8];
  char LinkFlag;
  bool DeviceMajorDefined;
  bool DeviceMinorDefined;

  bool IsDir() const
  {
    switch(LinkFlag)
    {
      case NFileHeader::NLinkFlag::kDirectory:
      case NFileHeader::NLinkFlag::kDumpDir:
        return true;
      case NFileHeader::NLinkFlag::kOldNormal:
      case NFileHeader::NLinkFlag::kNormal:
        return NItemName::HasTailSlash(Name, CP_OEMCP);
    }
    return false;
  }

  bool IsMagic() const
  {
    for (int i = 0; i < 5; i++)
      if (Magic[i] != NFileHeader::NMagic::kUsTar[i])
        return false;
    return true;
  }

  UInt64 GetPackSize() const { return (Size + 0x1FF) & (~((UInt64)0x1FF)); }
};

struct CItemEx: public CItem
{
  UInt64 HeaderPosition;
  unsigned LongLinkSize;
  UInt64 GetDataPosition() const { return HeaderPosition + LongLinkSize + NFileHeader::kRecordSize; }
  UInt64 GetFullSize() const { return LongLinkSize + NFileHeader::kRecordSize + Size; }
};

}}

#endif
