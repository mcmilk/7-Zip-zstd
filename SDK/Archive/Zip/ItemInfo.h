// Archive/Zip/ItemInfo.h

#pragma once

#ifndef __ARCHIVE_ZIP_ITEMINFO_H
#define __ARCHIVE_ZIP_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive {
namespace NZip {

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
  CVersion MadeByVersion;
  CVersion ExtractVersion;
  UINT16 Flags;
  UINT16 CompressionMethod;
  UINT32 Time;
  UINT32 FileCRC;
  UINT32 PackSize;
  UINT32 UnPackSize;
  UINT16 InternalAttributes;
  UINT32 ExternalAttributes;
  
  AString Name;
  
  UINT32 LocalHeaderPosition;
  UINT16 LocalExtraSize;
  UINT16 CentralExtraSize;
  UINT16 CommentSize;
  
  bool IsEncrypted() const;
  
  bool IsImplodeBigDictionary() const;
  bool IsImplodeLiteralsOn() const;
  
  bool IsDirectory() const;
  bool IgnoreItem() const { return false; }
  UINT32 GetWinAttributes() const;
  
  bool HasDescriptor() const;
  
  #ifdef WIN32
  WORD GetCodePage() const
    { return (MadeByVersion.HostOS == NFileHeader::NHostOS::kFAT) ? CP_OEMCP : CP_ACP; }
  #endif

private:
  void SetFlagBits(int aStartBitNumber, int aNumBits, int aValue);
  void SetBitMask(int aBitMask, bool anEnable);
public:
  void ClearFlags();
  void SetEncrypted(bool anEncrypted);
};

}}

#endif


