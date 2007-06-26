// Archive/ArjItem.h

#ifndef __ARCHIVE_ARJ_ITEM_H
#define __ARCHIVE_ARJ_ITEM_H

#include "Common/Types.h"
#include "Common/MyString.h"
#include "ArjHeader.h"

namespace NArchive {
namespace NArj {

struct CVersion
{
  Byte Version;
  Byte HostOS;
};

inline bool operator==(const CVersion &v1, const CVersion &v2)
  { return (v1.Version == v2.Version) && (v1.HostOS == v2.HostOS); }
inline bool operator!=(const CVersion &v1, const CVersion &v2)
  {  return !(v1 == v2); } 

class CItem
{
public:
  Byte Version;
  Byte ExtractVersion;
  Byte HostOS;
  Byte Flags;
  Byte Method;
  Byte FileType;
  UInt32 ModifiedTime;
  UInt32 PackSize;
  UInt32 Size;
  UInt32 FileCRC;

  // UInt16 FilespecPositionInFilename;
  UInt16 FileAccessMode;
  // Byte FirstChapter;
  // Byte LastChapter;
  
  AString Name;
  
  bool IsEncrypted() const { return (Flags & NFileHeader::NFlags::kGarbled) != 0; }
  bool IsDirectory() const { return (FileType == NFileHeader::NFileType::kDirectory); }
  UInt32 GetWinAttributes() const 
  {
    UInt32 winAtrributes;
    switch(HostOS)
    {
      case NFileHeader::NHostOS::kMSDOS:
      case NFileHeader::NHostOS::kWIN95:
        winAtrributes = FileAccessMode;
        break;
      default:
        winAtrributes = 0;
    }
    if (IsDirectory())
      winAtrributes |= FILE_ATTRIBUTE_DIRECTORY;
    return winAtrributes;
  }
};

class CItemEx: public CItem
{
public:
  UInt64 DataPosition;
};

}}

#endif


