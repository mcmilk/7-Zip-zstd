// RarItem.h

#ifndef __ARCHIVE_RAR_ITEM_H
#define __ARCHIVE_RAR_ITEM_H

#include "Common/Types.h"
#include "Common/MyString.h"

#include "RarHeader.h"

namespace NArchive{
namespace NRar{

struct CRarTime
{
  UInt32 DosTime;
  Byte LowSecond;
  Byte SubTime[3];
};

struct CItem
{
  UInt64 Size;
  UInt64 PackSize;
  
  CRarTime CTime;
  CRarTime ATime;
  CRarTime MTime;

  UInt32 FileCRC;
  UInt32 Attrib;

  UInt16 Flags;
  Byte HostOS;
  Byte UnPackVersion;
  Byte Method;

  bool CTimeDefined;
  bool ATimeDefined;

  AString Name;
  UString UnicodeName;

  Byte Salt[8];
  
  bool IsEncrypted()   const { return (Flags & NHeader::NFile::kEncrypted) != 0; }
  bool IsSolid()       const { return (Flags & NHeader::NFile::kSolid) != 0; }
  bool IsCommented()   const { return (Flags & NHeader::NFile::kComment) != 0; }
  bool IsSplitBefore() const { return (Flags & NHeader::NFile::kSplitBefore) != 0; }
  bool IsSplitAfter()  const { return (Flags & NHeader::NFile::kSplitAfter) != 0; }
  bool HasSalt()       const { return (Flags & NHeader::NFile::kSalt) != 0; }
  bool HasExtTime()    const { return (Flags & NHeader::NFile::kExtTime) != 0; }
  bool HasUnicodeName()const { return (Flags & NHeader::NFile::kUnicodeName) != 0; }
  bool IsOldVersion()  const { return (Flags & NHeader::NFile::kOldVersion) != 0; }
  
  UInt32 GetDictSize() const { return (Flags >> NHeader::NFile::kDictBitStart) & NHeader::NFile::kDictMask; }
  bool IsDir() const;
  bool IgnoreItem() const;
  UInt32 GetWinAttributes() const;
  
  CItem(): CTimeDefined(false), ATimeDefined(false) {}
};

class CItemEx: public CItem
{
public:
  UInt64 Position;
  UInt16 MainPartSize;
  UInt16 CommentSize;
  UInt16 AlignSize;
  UInt64 GetFullSize()  const { return MainPartSize + CommentSize + AlignSize + PackSize; };
  //  DWORD GetHeaderWithCommentSize()  const { return MainPartSize + CommentSize; };
  UInt64 GetCommentPosition() const { return Position + MainPartSize; };
  UInt64 GetDataPosition()    const { return GetCommentPosition() + CommentSize + AlignSize; };
};

}}

#endif
