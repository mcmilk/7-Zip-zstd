// RarItem.h

#ifndef __ARCHIVE_RAR_ITEM_H
#define __ARCHIVE_RAR_ITEM_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive{
namespace NRar{

struct CRarTime
{
  UInt32 DosTime;
  Byte LowSecond;
  Byte SubTime[3];
};

class CItem
{
public:
  UInt16 Flags;
  UInt64 PackSize;
  UInt64 UnPackSize;
  Byte HostOS;
  UInt32 FileCRC;
  
  CRarTime CreationTime;
  CRarTime LastWriteTime;
  CRarTime LastAccessTime;
  bool IsCreationTimeDefined;
  // bool IsLastWriteTimeDefined;
  bool IsLastAccessTimeDefined;

  Byte UnPackVersion;
  Byte Method;
  UInt32 Attributes;
  AString Name;
  UString UnicodeName;

  Byte Salt[8];
  
  bool IsEncrypted() const;
  bool IsSolid() const;
  bool IsCommented() const;
  bool IsSplitBefore() const;
  bool IsSplitAfter() const;
  bool HasSalt() const;
  bool HasExtTime() const;

  bool HasUnicodeName() const;
  bool IsOldVersion() const;
  
  UInt32 GetDictSize() const;
  bool IsDirectory() const;
  bool IgnoreItem() const;
  UInt32 GetWinAttributes() const;
  
  CItem(): IsCreationTimeDefined(false),  IsLastAccessTimeDefined(false) {}
private:
  void SetFlagBits(int aStartBitNumber, int aNumBits, int aValue);
  void SetBitMask(int aBitMask, bool anEnable);
public:
  void ClearFlags();
  void SetDictSize(UInt32 aSize);
  void SetAsDirectory(bool aDirectory);
  void SetEncrypted(bool anEncrypted);
  void SetSolid(bool aSolid);
  void SetCommented(bool aCommented);
  void SetSplitBefore(bool aSplitBefore);
  void SetSplitAfter(bool aSplitAfter);
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


