// Archive::Rar::ItemInfo.h

#pragma once

#ifndef __ARCHIVE_RAR_ITEMINFO_H
#define __ARCHIVE_RAR_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"

namespace NArchive{
namespace NRar{

struct CRarTime
{
  UINT32 DosTime;
  BYTE LowSecond;
  BYTE SubTime[3];
};

class CItemInfo
{
public:
  UINT16 Flags;
  UINT64 PackSize;
  UINT64 UnPackSize;
  BYTE HostOS;
  UINT32 FileCRC;
  
  CRarTime CreationTime;
  CRarTime LastWriteTime;
  CRarTime LastAccessTime;
  bool IsCreationTimeDefined;
  // bool IsLastWriteTimeDefined;
  bool IsLastAccessTimeDefined;

  BYTE UnPackVersion;
  BYTE Method;
  UINT32 Attributes;
  AString Name;
  UString UnicodeName;

  BYTE Salt[8];
  
  bool IsEncrypted() const;
  bool IsSolid() const;
  bool IsCommented() const;
  bool IsSplitBefore() const;
  bool IsSplitAfter() const;
  bool HasSalt() const;
  bool HasExtTime() const;

  bool HasUnicodeName() const;
  bool IsOldVersion() const;
  
  UINT32 GetDictSize() const;
  bool IsDirectory() const;
  bool IgnoreItem() const;
  UINT32 GetWinAttributes() const;
  
  CItemInfo(): IsCreationTimeDefined(false),  IsLastAccessTimeDefined(false) {}
private:
  void SetFlagBits(int aStartBitNumber, int aNumBits, int aValue);
  void SetBitMask(int aBitMask, bool anEnable);
public:
  void ClearFlags();
  void SetDictSize(UINT32 aSize);
  void SetAsDirectory(bool aDirectory);
  void SetEncrypted(bool anEncrypted);
  void SetSolid(bool aSolid);
  void SetCommented(bool aCommented);
  void SetSplitBefore(bool aSplitBefore);
  void SetSplitAfter(bool aSplitAfter);
};

}}

#endif


