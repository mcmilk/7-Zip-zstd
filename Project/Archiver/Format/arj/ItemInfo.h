// Archive/arj/ItemInfo.h

#pragma once

#ifndef __ARCHIVE_arj_ITEMINFO_H
#define __ARCHIVE_arj_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive {
namespace Narj {

struct CVersion
{
  BYTE Version;
  BYTE HostOS;
};

bool operator==(const CVersion &aVersion1, const CVersion &aVersion2);
bool operator!=(const CVersion &aVersion1, const CVersion &aVersion2);

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
  
  bool IsEncrypted() const;
  bool IsDirectory() const;
  UINT32 GetWinAttributes() const;
};

}}

#endif


