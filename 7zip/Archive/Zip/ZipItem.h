// Archive/ZipItem.h

#pragma once

#ifndef __ARCHIVE_ZIP_ITEM_H
#define __ARCHIVE_ZIP_ITEM_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive {
namespace NZip {

struct CVersion
{
  BYTE Version;
  BYTE HostOS;
};

bool operator==(const CVersion &v1, const CVersion &v2);
bool operator!=(const CVersion &v1, const CVersion &v2);

class CItem
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
  void SetFlagBits(int startBitNumber, int numBits, int value);
  void SetBitMask(int bitMask, bool enable);
public:
  void ClearFlags();
  void SetEncrypted(bool encrypted);
};

}}

#endif


