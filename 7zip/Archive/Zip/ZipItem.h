// Archive/ZipItem.h

#ifndef __ARCHIVE_ZIP_ITEM_H
#define __ARCHIVE_ZIP_ITEM_H

#include "Common/Types.h"
#include "Common/String.h"
#include "Common/Buffer.h"

#include "ZipHeader.h"

namespace NArchive {
namespace NZip {

struct CVersion
{
  Byte Version;
  Byte HostOS;
};

bool operator==(const CVersion &v1, const CVersion &v2);
bool operator!=(const CVersion &v1, const CVersion &v2);

struct CExtraSubBlock
{
  UInt16 ID;
  CByteBuffer Data;
};

struct CExtraBlock
{
  CObjectVector<CExtraSubBlock> SubBlocks;
  void Clear() { SubBlocks.Clear(); }
};


class CItem
{
public:
  CVersion MadeByVersion;
  CVersion ExtractVersion;
  UInt16 Flags;
  UInt16 CompressionMethod;
  UInt32 Time;
  UInt32 FileCRC;
  UInt64 PackSize;
  UInt64 UnPackSize;
  UInt16 InternalAttributes;
  UInt32 ExternalAttributes;
  
  AString Name;
  
  UInt64 LocalHeaderPosition;
  UInt16 LocalExtraSize;
  
  CExtraBlock CentralExtra;
  CByteBuffer Comment;
  
  bool IsEncrypted() const;
  
  bool IsImplodeBigDictionary() const;
  bool IsImplodeLiteralsOn() const;
  
  bool IsDirectory() const;
  bool IgnoreItem() const { return false; }
  UInt32 GetWinAttributes() const;
  
  bool HasDescriptor() const;
  
  WORD GetCodePage() const
  {
    return (MadeByVersion.HostOS == NFileHeader::NHostOS::kFAT 
        || MadeByVersion.HostOS == NFileHeader::NHostOS::kNTFS
        ) ? CP_OEMCP : CP_ACP;
  }

private:
  void SetFlagBits(int startBitNumber, int numBits, int value);
  void SetBitMask(int bitMask, bool enable);
public:
  void ClearFlags();
  void SetEncrypted(bool encrypted);
};

}}

#endif


