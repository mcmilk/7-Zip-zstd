// Archive/Arj/ItemInfo.h

#pragma once

#ifndef __ARCHIVE_ARJ_ITEMINFO_H
#define __ARCHIVE_ARJ_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"
#include "Header.h"

namespace NArchive {
namespace NArj {

struct CVersion
{
  BYTE Version;
  BYTE HostOS;
};

inline bool operator==(const CVersion &v1, const CVersion &v2)
  { return (v1.Version == v2.Version) && (v1.HostOS == v2.HostOS); }
inline bool operator!=(const CVersion &v1, const CVersion &v2)
  {  return !(v1 == v2); } 

class CItemInfo
{
public:
  BYTE Version;
  BYTE ExtractVersion;
  BYTE HostOS;
  BYTE Flags;
  BYTE Method;
  BYTE FileType;
  UINT32 ModifiedTime;
  UINT32 PackSize;
  UINT32 Size;
  UINT32 FileCRC;

  // UINT16 FilespecPositionInFilename;
  UINT16 FileAccessMode;
  // BYTE FirstChapter;
  // BYTE LastChapter;
  
  AString Name;
  
  bool IsEncrypted() const { return (Flags & NFileHeader::NFlags::kGarbled) != 0; }
  bool IsDirectory() const { return (FileType == NFileHeader::NFileType::kDirectory); }
  UINT32 GetWinAttributes() const 
  {
    DWORD winAtrributes;
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
;
};

}}

#endif


