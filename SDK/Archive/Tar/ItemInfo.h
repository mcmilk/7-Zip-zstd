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
    {  return (LinkFlag == NFileHeader::NLinkFlag::kDirectory); }
};

}}

#endif
