// Archive/GZip/ItemInfo.h

#pragma once

#ifndef __ARCHIVE_GZIP_ITEMINFO_H
#define __ARCHIVE_GZIP_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive {
namespace NGZip {

class CItemInfo
{
private:
  bool TestFlag(BYTE aFlag) const { return ((Flags & aFlag) != 0); }
public:
  BYTE CompressionMethod;
  BYTE Flags;
  UINT32 Time;
  BYTE ExtraFlags;
  BYTE HostOS;
  UINT32 FileCRC;
  UINT32 UnPackSize32;
  UINT64 PackSize;

  AString Name;
  UINT16 ExtraFieldSize;
  UINT16 CommentSize;

  bool IsText() const;
  bool HeaderCRCIsPresent() const;
  bool ExtraFieldIsPresent() const;
  bool NameIsPresent() const;
  bool CommentIsPresent() const;

  void SetNameIsPresentFlag(bool aNameIsPresent);
};

}}

#endif


