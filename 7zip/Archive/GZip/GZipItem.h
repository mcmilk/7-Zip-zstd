// Archive/GZipItem.h

#pragma once

#ifndef __ARCHIVE_GZIP_ITEM_H
#define __ARCHIVE_GZIP_ITEM_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive {
namespace NGZip {

class CItem
{
private:
  bool TestFlag(BYTE flag) const { return ((Flags & flag) != 0); }
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

  bool IsText() const
    {  return TestFlag(NFileHeader::NFlags::kDataIsText); }
  bool HeaderCRCIsPresent() const
    {  return TestFlag(NFileHeader::NFlags::kHeaderCRCIsPresent); }
  bool ExtraFieldIsPresent() const
    {  return TestFlag(NFileHeader::NFlags::kExtraIsPresent); }
  bool NameIsPresent() const
    {  return TestFlag(NFileHeader::NFlags::kNameIsPresent); }
  bool CommentIsPresent() const
    {  return TestFlag(NFileHeader::NFlags::kComentIsPresent); }

  void SetNameIsPresentFlag(bool nameIsPresent)
  {    
    if (nameIsPresent)
      Flags |= NFileHeader::NFlags::kNameIsPresent;
    else
      Flags &= (~NFileHeader::NFlags::kNameIsPresent);
  }
};

class CItemEx: public CItem
{
public:
  UINT64 DataPosition;
  UINT64 CommentPosition;
  UINT64 ExtraPosition;
};


}}

#endif


