// Archive/GZipItem.h

#ifndef __ARCHIVE_GZIP_ITEM_H
#define __ARCHIVE_GZIP_ITEM_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive {
namespace NGZip {

class CItem
{
private:
  bool TestFlag(Byte flag) const { return ((Flags & flag) != 0); }
public:
  Byte CompressionMethod;
  Byte Flags;
  UInt32 Time;
  Byte ExtraFlags;
  Byte HostOS;
  UInt32 FileCRC;
  UInt32 UnPackSize32;
  UInt64 PackSize;

  AString Name;
  UInt16 ExtraFieldSize;
  UInt16 CommentSize;

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
  UInt64 DataPosition;
  UInt64 CommentPosition;
  UInt64 ExtraPosition;
};


}}

#endif


