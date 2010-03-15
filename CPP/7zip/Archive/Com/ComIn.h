// Archive/ComIn.h

#ifndef __ARCHIVE_COM_IN_H
#define __ARCHIVE_COM_IN_H

#include "Common/MyString.h"
#include "Common/Buffer.h"

namespace NArchive {
namespace NCom {

struct CUInt32Buf
{
  UInt32 *_buf;
public:
  CUInt32Buf(): _buf(0) {}
  ~CUInt32Buf() { Free(); }
  void Free();
  bool Allocate(UInt32 numItems);
  operator UInt32 *() const { return _buf; };
};

namespace NFatID
{
  const UInt32 kFree       = 0xFFFFFFFF;
  const UInt32 kEndOfChain = 0xFFFFFFFE;
  const UInt32 kFatSector  = 0xFFFFFFFD;
  const UInt32 kMatSector  = 0xFFFFFFFC;
  const UInt32 kMaxValue   = 0xFFFFFFFA;
}

namespace NItemType
{
  const Byte kEmpty = 0;
  const Byte kStorage = 1;
  const Byte kStream = 2;
  const Byte kLockBytes = 3;
  const Byte kProperty = 4;
  const Byte kRootStorage = 5;
}

const UInt32 kNameSizeMax = 64;

struct CItem
{
  Byte Name[kNameSizeMax];
  // UInt16 NameSize;
  // UInt32 Flags;
  FILETIME CTime;
  FILETIME MTime;
  UInt64 Size;
  UInt32 LeftDid;
  UInt32 RightDid;
  UInt32 SonDid;
  UInt32 Sid;
  Byte Type;

  bool IsEmpty() const { return Type == NItemType::kEmpty; }
  bool IsDir() const { return Type == NItemType::kStorage || Type == NItemType::kRootStorage; }

  void Parse(const Byte *p, bool mode64bit);
};

struct CRef
{
  int Parent;
  UInt32 Did;
};

class CDatabase
{
  UInt32 NumSectorsInMiniStream;
  CUInt32Buf MiniSids;

  HRESULT AddNode(int parent, UInt32 did);
public:

  CUInt32Buf Fat;
  UInt32 FatSize;
  
  CUInt32Buf Mat;
  UInt32 MatSize;

  CObjectVector<CItem> Items;
  CRecordVector<CRef> Refs;

  UInt32 LongStreamMinSize;
  int SectorSizeBits;
  int MiniSectorSizeBits;

  Int32 MainSubfile;

  void Clear();
  bool IsLargeStream(UInt64 size) const { return size >= LongStreamMinSize; }
  UString GetItemPath(UInt32 index) const;

  UInt64 GetItemPackSize(UInt64 size) const
  {
    UInt64 mask = ((UInt64)1 << (IsLargeStream(size) ? SectorSizeBits : MiniSectorSizeBits)) - 1;
    return (size + mask) & ~mask;
  }

  bool GetMiniCluster(UInt32 sid, UInt64 &res) const
  {
    int subBits = SectorSizeBits - MiniSectorSizeBits;
    UInt32 fid = sid >> subBits;
    if (fid >= NumSectorsInMiniStream)
      return false;
    res = (((UInt64)MiniSids[fid] + 1) << subBits) + (sid & ((1 << subBits) - 1));
    return true;
  }

  HRESULT Open(IInStream *inStream);
};


}}
  
#endif
