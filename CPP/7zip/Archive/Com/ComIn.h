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
  FILETIME CreationTime;
  FILETIME LastWriteTime;
  UInt64 Size;
  UInt32 LeftDid;
  UInt32 RightDid;
  UInt32 SonDid;
  UInt32 Sid;
  Byte Type;

  bool IsEmpty() const { return Type == NItemType::kEmpty; }
  bool IsDir() const { return Type == NItemType::kStorage || Type == NItemType::kRootStorage; }
};

struct CRef
{
  int Parent;
  UInt32 Did;
};

class CDatabase
{
public:
  HRESULT AddNode(int parent, UInt32 did);

  CUInt32Buf Fat;
  UInt32 FatSize;
  
  CUInt32Buf MiniSids;
  UInt32 NumSectorsInMiniStream;

  CUInt32Buf Mat;
  UInt32 MatSize;

  CObjectVector<CItem> Items;
  CRecordVector<CRef> Refs;

  UInt32 LongStreamMinSize;
  int SectorSizeBits;
  int MiniSectorSizeBits;

  void Clear()
  {
    Fat.Free();
    MiniSids.Free();
    Mat.Free();
    Items.Clear();
    Refs.Clear();
  }

  bool IsLargeStream(UInt64 size) { return size >= LongStreamMinSize; }
  UString GetItemPath(UInt32 index) const;
};

HRESULT OpenArchive(IInStream *inStream, CDatabase &database);

}}
  
#endif
