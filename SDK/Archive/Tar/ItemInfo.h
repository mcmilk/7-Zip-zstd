// Archive/Tar/ItemInfo.h

#pragma once

#ifndef __ARCHIVE_TAR_ITEMINFO_H
#define __ARCHIVE_TAR_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"
#include "Archive/Tar/Header.h"

namespace NArchive {
namespace NTar {

class CItemInfo
{
public:
  AString Name;
  UINT32 Mode;
  UINT32 UID;
  UINT32 GID;
  UINT64 Size;
  time_t ModificationTime;
  char LinkFlag;
  AString LinkName;
  char Magic[8];
  AString UserName;
  AString GroupName;

  bool DeviceMajorDefined;
  UINT32 DeviceMajor;
  bool DeviceMinorDefined;
  UINT32 DeviceMinor;

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

}}

#endif
