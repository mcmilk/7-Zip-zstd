// Archive/Tar/Item.h

#ifndef __ARCHIVE_TAR_ITEM_H
#define __ARCHIVE_TAR_ITEM_H

#include <time.h>

#include "Common/Types.h"
#include "Common/String.h"
#include "TarHeader.h"

namespace NArchive {
namespace NTar {

class CItem
{
public:
  AString Name;
  UInt32 Mode;
  UInt32 UID;
  UInt32 GID;
  UInt64 Size;
  time_t ModificationTime;
  char LinkFlag;
  AString LinkName;
  char Magic[8];
  AString UserName;
  AString GroupName;

  bool DeviceMajorDefined;
  UInt32 DeviceMajor;
  bool DeviceMinorDefined;
  UInt32 DeviceMinor;

  bool IsDirectory() const 
    {  
      if (LinkFlag == NFileHeader::NLinkFlag::kDirectory)
        return true;
      if (LinkFlag == NFileHeader::NLinkFlag::kOldNormal || 
          LinkFlag == NFileHeader::NLinkFlag::kNormal)
      {
        if (Name.IsEmpty())
          return false;
        #ifdef WIN32
        return (*CharPrevExA(CP_OEMCP, Name, &Name[Name.Length()], 0) == '/');
        #else
        return (Name[Name.Length() - 1) == '/');
        #endif
      }
      return false;
    }
};

class CItemEx: public CItem
{
public:
  UInt64 HeaderPosition;
  UInt64 LongLinkSize;
  UInt64 GetDataPosition() const { return HeaderPosition + LongLinkSize + NFileHeader::kRecordSize; };
  UInt64 GetFullSize() const { return LongLinkSize + NFileHeader::kRecordSize + Size; };
};

}}

#endif
