// Archive/ZipItem.h

#ifndef __ARCHIVE_ZIP_ITEM_H
#define __ARCHIVE_ZIP_ITEM_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive {
namespace NZip {

struct CVersion
{
  Byte Version;
  Byte HostOS;
};

bool operator==(const CVersion &v1, const CVersion &v2);
bool operator!=(const CVersion &v1, const CVersion &v2);

class CItem
{
public:
  CVersion MadeByVersion;
  CVersion ExtractVersion;
  UInt16 Flags;
  UInt16 CompressionMethod;
  UInt32 Time;
  UInt32 FileCRC;
  UInt32 PackSize;
  UInt32 UnPackSize;
  UInt16 InternalAttributes;
  UInt32 ExternalAttributes;
  
  AString Name;
  
  UInt32 LocalHeaderPosition;
  UInt16 LocalExtraSize;
  UInt16 CentralExtraSize;
  UInt16 CommentSize;
  
  bool IsEncrypted() const;
  
  bool IsImplodeBigDictionary() const;
  bool IsImplodeLiteralsOn() const;
  
  bool IsDirectory() const;
  bool IgnoreItem() const { return false; }
  UInt32 GetWinAttributes() const;
  
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


