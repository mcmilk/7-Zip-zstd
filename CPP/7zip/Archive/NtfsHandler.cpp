// NtfsHandler.cpp

#include "StdAfx.h"

// #define SHOW_DEBUG_INFO
// #define SHOW_DEBUG_INFO2

#if defined(SHOW_DEBUG_INFO) || defined(SHOW_DEBUG_INFO2)
#include <stdio.h>
#endif

#include "../../../C/CpuArch.h"

#include "Common/Buffer.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/MyCom.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"

#include "Common/DummyOutStream.h"

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

#ifdef SHOW_DEBUG_INFO2
#define PRF2(x) x
#else
#define PRF2(x)
#endif

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

#define G16(p, dest) dest = Get16(p);
#define G32(p, dest) dest = Get32(p);
#define G64(p, dest) dest = Get64(p);

namespace NArchive {
namespace Ntfs {

static const UInt32 kNumSysRecs = 16;
static const UInt32 kRecIndex_Volume = 3;
static const UInt32 kRecIndex_BadClus = 8;

struct CHeader
{
  Byte SectorSizeLog;
  Byte ClusterSizeLog;
  // Byte MediaType;
  UInt32 NumHiddenSectors;
  UInt64 NumClusters;
  UInt64 MftCluster;
  UInt64 SerialNumber;
  UInt16 SectorsPerTrack;
  UInt16 NumHeads;

  UInt64 GetPhySize() const { return NumClusters << ClusterSizeLog; }
  UInt32 ClusterSize() const { return (UInt32)1 << ClusterSizeLog; }
  bool Parse(const Byte *p);
};

static int GetLog(UInt32 num)
{
  for (int i = 0; i < 31; i++)
    if (((UInt32)1 << i) == num)
      return i;
  return -1;
}

bool CHeader::Parse(const Byte *p)
{
  if (p[0x1FE] != 0x55 || p[0x1FF] != 0xAA)
    return false;

  int codeOffset = 0;
  switch (p[0])
  {
    case 0xE9: codeOffset = 3 + (Int16)Get16(p + 1); break;
    case 0xEB: if (p[2] != 0x90) return false; codeOffset = 2 + (signed char)p[1]; break;
    default: return false;
  }
  Byte sectorsPerClusterLog;

  if (memcmp(p + 3, "NTFS    ", 8) != 0)
    return false;
  {
    int s = GetLog(Get16(p + 11));
    if (s < 9 || s > 12)
      return false;
    SectorSizeLog = (Byte)s;
    s = GetLog(p[13]);
    if (s < 0)
      return false;
    sectorsPerClusterLog = (Byte)s;
    ClusterSizeLog = SectorSizeLog + sectorsPerClusterLog;
  }

  for (int i = 14; i < 21; i++)
    if (p[i] != 0)
      return false;

  // MediaType = p[21];
  if (Get16(p + 22) != 0) // NumFatSectors
    return false;
  G16(p + 24, SectorsPerTrack);
  G16(p + 26, NumHeads);
  G32(p + 28, NumHiddenSectors);
  if (Get32(p + 32) != 0) // NumSectors32
    return false;

  // DriveNumber = p[0x24];
  if (p[0x25] != 0) // CurrentHead
    return false;
  /*
  NTFS-HDD:   p[0x26] = 0x80
  NTFS-FLASH: p[0x26] = 0
  */
  if (p[0x26] != 0x80 && p[0x26] != 0) // ExtendedBootSig
    return false;
  if (p[0x27] != 0) // reserved
    return false;
  UInt64 numSectors = Get64(p + 0x28);
  NumClusters = numSectors >> sectorsPerClusterLog;

  G64(p + 0x30, MftCluster);
  // G64(p + 0x38, Mft2Cluster);
  G64(p + 0x48, SerialNumber);
  UInt32 numClustersInMftRec;
  UInt32 numClustersInIndexBlock;
  G32(p + 0x40, numClustersInMftRec);
  G32(p + 0x44, numClustersInIndexBlock);
  return (numClustersInMftRec < 256 && numClustersInIndexBlock < 256);
}

struct CMftRef
{
  UInt64 Val;
  UInt64 GetIndex() const { return Val & (((UInt64)1 << 48) - 1); }
  UInt16 GetNumber() const { return (UInt16)(Val >> 48); }
  bool IsBaseItself() const { return Val == 0; }
};

#define ATNAME(n) ATTR_TYPE_ ## n
#define DEF_ATTR_TYPE(v, n) ATNAME(n) = v

enum
{
  DEF_ATTR_TYPE(0x00, UNUSED),
  DEF_ATTR_TYPE(0x10, STANDARD_INFO),
  DEF_ATTR_TYPE(0x20, ATTRIBUTE_LIST),
  DEF_ATTR_TYPE(0x30, FILE_NAME),
  DEF_ATTR_TYPE(0x40, OBJECT_ID),
  DEF_ATTR_TYPE(0x50, SECURITY_DESCRIPTOR),
  DEF_ATTR_TYPE(0x60, VOLUME_NAME),
  DEF_ATTR_TYPE(0x70, VOLUME_INFO),
  DEF_ATTR_TYPE(0x80, DATA),
  DEF_ATTR_TYPE(0x90, INDEX_ROOT),
  DEF_ATTR_TYPE(0xA0, INDEX_ALLOCATION),
  DEF_ATTR_TYPE(0xB0, BITMAP),
  DEF_ATTR_TYPE(0xC0, REPARSE_POINT),
  DEF_ATTR_TYPE(0xD0, EA_INFO),
  DEF_ATTR_TYPE(0xE0, EA),
  DEF_ATTR_TYPE(0xF0, PROPERTY_SET),
  DEF_ATTR_TYPE(0x100, LOGGED_UTILITY_STREAM),
  DEF_ATTR_TYPE(0x1000, FIRST_USER_DEFINED_ATTRIBUTE)
};

static const Byte kFileNameType_Posix = 0;
static const Byte kFileNameType_Win32 = 1;
static const Byte kFileNameType_Dos = 2;
static const Byte kFileNameType_Win32Dos = 3;

struct CFileNameAttr
{
  CMftRef ParentDirRef;
  // UInt64 CTime;
  // UInt64 MTime;
  // UInt64 ThisRecMTime;
  // UInt64 ATime;
  // UInt64 AllocatedSize;
  // UInt64 DataSize;
  // UInt16 PackedEaSize;
  UString Name;
  UInt32 Attrib;
  Byte NameType;
  
  bool IsDos() const { return NameType == kFileNameType_Dos; }
  bool Parse(const Byte *p, unsigned size);
};

static void GetString(const Byte *p, unsigned length, UString &res)
{
  wchar_t *s = res.GetBuffer(length);
  for (unsigned i = 0; i < length; i++)
    s[i] = Get16(p + i * 2);
  s[length] = 0;
  res.ReleaseBuffer();
}

bool CFileNameAttr::Parse(const Byte *p, unsigned size)
{
  if (size < 0x42)
    return false;
  G64(p + 0x00, ParentDirRef.Val);
  // G64(p + 0x08, CTime);
  // G64(p + 0x10, MTime);
  // G64(p + 0x18, ThisRecMTime);
  // G64(p + 0x20, ATime);
  // G64(p + 0x28, AllocatedSize);
  // G64(p + 0x30, DataSize);
  G32(p + 0x38, Attrib);
  // G16(p + 0x3C, PackedEaSize);
  NameType = p[0x41];
  unsigned length = p[0x40];
  if (0x42 + length > size)
    return false;
  GetString(p + 0x42, length, Name);
  return true;
}

struct CSiAttr
{
  UInt64 CTime;
  UInt64 MTime;
  // UInt64 ThisRecMTime;
  UInt64 ATime;
  UInt32 Attrib;

  /*
  UInt32 MaxVersions;
  UInt32 Version;
  UInt32 ClassId;
  UInt32 OwnerId;
  UInt32 SecurityId;
  UInt64 QuotaCharged;
  */

  bool Parse(const Byte *p, unsigned size);
};

bool CSiAttr::Parse(const Byte *p, unsigned size)
{
  if (size < 0x24)
    return false;
  G64(p + 0x00, CTime);
  G64(p + 0x08, MTime);
  // G64(p + 0x10, ThisRecMTime);
  G64(p + 0x18, ATime);
  G32(p + 0x20, Attrib);
  return true;
}

static const UInt64 kEmptyExtent = (UInt64)(Int64)-1;

struct CExtent
{
  UInt64 Virt;
  UInt64 Phy;

  bool IsEmpty() const { return Phy == kEmptyExtent; }
};

struct CVolInfo
{
  Byte MajorVer;
  Byte MinorVer;
  // UInt16 Flags;

  bool Parse(const Byte *p, unsigned size);
};

bool CVolInfo::Parse(const Byte *p, unsigned size)
{
  if (size < 12)
    return false;
  MajorVer = p[8];
  MinorVer = p[9];
  // Flags = Get16(p + 10);
  return true;
}

struct CAttr
{
  UInt32 Type;
  // UInt32 Length;
  UString Name;
  // UInt16 Flags;
  // UInt16 Instance;
  CByteBuffer Data;
  Byte NonResident;

  // Non-Resident
  Byte CompressionUnit;
  UInt64 LowVcn;
  UInt64 HighVcn;
  UInt64 AllocatedSize;
  UInt64 Size;
  UInt64 PackSize;
  UInt64 InitializedSize;

  // Resident
  // UInt16 ResidentFlags;

  bool IsCompressionUnitSupported() const { return CompressionUnit == 0 || CompressionUnit == 4; }

  UInt32 Parse(const Byte *p, unsigned size);
  bool ParseFileName(CFileNameAttr &a) const { return a.Parse(Data, (unsigned)Data.GetCapacity()); }
  bool ParseSi(CSiAttr &a) const { return a.Parse(Data, (unsigned)Data.GetCapacity()); }
  bool ParseVolInfo(CVolInfo &a) const { return a.Parse(Data, (unsigned)Data.GetCapacity()); }
  bool ParseExtents(CRecordVector<CExtent> &extents, UInt64 numClustersMax, int compressionUnit) const;
  UInt64 GetSize() const { return NonResident ? Size : Data.GetCapacity(); }
  UInt64 GetPackSize() const
  {
    if (!NonResident)
      return Data.GetCapacity();
    if (CompressionUnit != 0)
      return PackSize;
    return AllocatedSize;
  }
};

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

static int CompareAttr(void *const *elem1, void *const *elem2, void *)
{
  const CAttr &a1 = *(*((const CAttr **)elem1));
  const CAttr &a2 = *(*((const CAttr **)elem2));
  RINOZ(MyCompare(a1.Type, a2.Type));
  RINOZ(MyCompare(a1.Name, a2.Name));
  return MyCompare(a1.LowVcn, a2.LowVcn);
}

UInt32 CAttr::Parse(const Byte *p, unsigned size)
{
  if (size < 4)
    return 0;
  G32(p, Type);
  if (Type == 0xFFFFFFFF)
    return 4;
  if (size < 0x18)
    return 0;
  PRF(printf(" T=%2X", Type));
  
  UInt32 length = Get32(p + 0x04);
  PRF(printf(" L=%3d", length));
  if (length > size)
    return 0;
  NonResident = p[0x08];
  {
    int nameLength = p[9];
    UInt32 nameOffset = Get16(p + 0x0A);
    if (nameLength != 0)
    {
      if (nameOffset + nameLength * 2 > length)
        return 0;
      GetString(p + nameOffset, nameLength, Name);
      PRF(printf(" N=%S", Name));
    }
  }

  // G16(p + 0x0C, Flags);
  // G16(p + 0x0E, Instance);
  // PRF(printf(" F=%4X", Flags));
  // PRF(printf(" Inst=%d", Instance));

  UInt32 dataSize;
  UInt32 offs;
  if (NonResident)
  {
    if (length < 0x40)
      return 0;
    PRF(printf(" NR"));
    G64(p + 0x10, LowVcn);
    G64(p + 0x18, HighVcn);
    G64(p + 0x28, AllocatedSize);
    G64(p + 0x30, Size);
    G64(p + 0x38, InitializedSize);
    G16(p + 0x20, offs);
    CompressionUnit = p[0x22];

    PackSize = Size;
    if (CompressionUnit != 0)
    {
      if (length < 0x48)
        return 0;
      G64(p + 0x40, PackSize);
      PRF(printf(" PS=%I64x", PackSize));
    }

    // PRF(printf("\n"));
    PRF(printf(" ASize=%4I64d", AllocatedSize));
    PRF(printf(" Size=%I64d", Size));
    PRF(printf(" IS=%I64d", InitializedSize));
    PRF(printf(" Low=%I64d", LowVcn));
    PRF(printf(" High=%I64d", HighVcn));
    PRF(printf(" CU=%d", (int)CompressionUnit));
    dataSize = length - offs;
  }
  else
  {
    if (length < 0x18)
      return 0;
    PRF(printf(" RES"));
    dataSize = Get32(p + 0x10);
    PRF(printf(" dataSize=%3d", dataSize));
    offs = Get16(p + 0x14);
    // G16(p + 0x16, ResidentFlags);
    // PRF(printf(" ResFlags=%4X", ResidentFlags));
  }
  if (offs > length || dataSize > length || length - dataSize < offs)
    return 0;
  Data.SetCapacity(dataSize);
  memcpy(Data, p + offs, dataSize);
  #ifdef SHOW_DEBUG_INFO
  PRF(printf("  : "));
  for (unsigned i = 0; i < Data.GetCapacity(); i++)
  {
    PRF(printf(" %02X", (int)Data[i]));
  }
  #endif
  return length;
}

bool CAttr::ParseExtents(CRecordVector<CExtent> &extents, UInt64 numClustersMax, int compressionUnit) const
{
  const Byte *p = Data;
  unsigned size = (unsigned)Data.GetCapacity();
  UInt64 vcn = LowVcn;
  UInt64 lcn = 0;
  UInt64 highVcn1 = HighVcn + 1;
  if (LowVcn != extents.Back().Virt || highVcn1 > (UInt64)1 << 63)
    return false;

  extents.DeleteBack();

  PRF2(printf("\n# ParseExtents # LowVcn = %4I64X # HighVcn = %4I64X", LowVcn, HighVcn));

  while (size > 0)
  {
    Byte b = *p++;
    size--;
    if (b == 0)
      break;
    UInt32 num = b & 0xF;
    if (num == 0 || num > 8 || num > size)
      return false;

    int i;
    UInt64 vSize = p[num - 1];
    for (i = (int)num - 2; i >= 0; i--)
      vSize = (vSize << 8) | p[i];
    if (vSize == 0)
      return false;
    p += num;
    size -= num;
    if ((highVcn1 - vcn) < vSize)
      return false;

    num = (b >> 4) & 0xF;
    if (num > 8 || num > size)
      return false;
    CExtent e;
    e.Virt = vcn;
    if (num == 0)
    {
      if (compressionUnit == 0)
        return false;
      e.Phy = kEmptyExtent;
    }
    else
    {
      Int64 v = (signed char)p[num - 1];
      for (i = (int)num - 2; i >= 0; i--)
        v = (v << 8) | p[i];
      p += num;
      size -= num;
      lcn += v;
      if (lcn > numClustersMax)
        return false;
      e.Phy = lcn;
    }
    extents.Add(e);
    vcn += vSize;
  }
  CExtent e;
  e.Phy = kEmptyExtent;
  e.Virt = vcn;
  extents.Add(e);
  return (highVcn1 == vcn);
}

static const UInt64 kEmptyTag = (UInt64)(Int64)-1;

static const int kNumCacheChunksLog = 1;
static const UInt32 kNumCacheChunks = (1 << kNumCacheChunksLog);

class CInStream:
  public IInStream,
  public CMyUnknownImp
{
  UInt64 _virtPos;
  UInt64 _physPos;
  UInt64 _curRem;
  bool _sparseMode;
  size_t _compressedPos;
  
  UInt64 _tags[kNumCacheChunks];
  int _chunkSizeLog;
  CByteBuffer _inBuf;
  CByteBuffer _outBuf;
public:
  CMyComPtr<IInStream> Stream;
  UInt64 Size;
  UInt64 InitializedSize;
  int BlockSizeLog;
  int CompressionUnit;
  bool InUse;
  CRecordVector<CExtent> Extents;

  HRESULT SeekToPhys() { return Stream->Seek(_physPos, STREAM_SEEK_SET, NULL); }

  UInt32 GetCuSize() const { return (UInt32)1 << (BlockSizeLog + CompressionUnit); }
  HRESULT InitAndSeek(int compressionUnit)
  {
    CompressionUnit = compressionUnit;
    if (compressionUnit != 0)
    {
      UInt32 cuSize = GetCuSize();
      _inBuf.SetCapacity(cuSize);
      _chunkSizeLog = BlockSizeLog + CompressionUnit;
      _outBuf.SetCapacity(kNumCacheChunks << _chunkSizeLog);
    }
    for (int i = 0; i < kNumCacheChunks; i++)
      _tags[i] = kEmptyTag;

    _sparseMode = false;
    _curRem = 0;
    _virtPos = 0;
    _physPos = 0;
    const CExtent &e = Extents[0];
    if (!e.IsEmpty())
      _physPos = e.Phy << BlockSizeLog;
    return SeekToPhys();
  }

  MY_UNKNOWN_IMP1(IInStream)

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
};

static size_t Lznt1Dec(Byte *dest, size_t outBufLim, size_t destLen, const Byte *src, size_t srcLen)
{
  size_t destSize = 0;
  while (destSize < destLen)
  {
    if (srcLen < 2 || (destSize & 0xFFF) != 0)
      break;
    UInt32 v = Get16(src);
    if (v == 0)
      break;
    src += 2;
    srcLen -= 2;
    UInt32 comprSize = (v & 0xFFF) + 1;
    if (comprSize > srcLen)
      break;
    srcLen -= comprSize;
    if ((v & 0x8000) == 0)
    {
      if (comprSize != (1 << 12))
        break;
      memcpy(dest + destSize, src, comprSize);
      src += comprSize;
      destSize += comprSize;
    }
    else
    {
      if (destSize + (1 << 12) > outBufLim || (src[0] & 1) != 0)
        return 0;
      int numDistBits = 4;
      UInt32 sbOffset = 0;
      UInt32 pos = 0;

      do
      {
        comprSize--;
        for (UInt32 mask = src[pos++] | 0x100; mask > 1 && comprSize > 0; mask >>= 1)
        {
          if ((mask & 1) == 0)
          {
            if (sbOffset >= (1 << 12))
              return 0;
            dest[destSize++] = src[pos++];
            sbOffset++;
            comprSize--;
          }
          else
          {
            if (comprSize < 2)
              return 0;
            UInt32 v = Get16(src + pos);
            pos += 2;
            comprSize -= 2;

            while (((sbOffset - 1) >> numDistBits) != 0)
              numDistBits++;

            UInt32 len = (v & (0xFFFF >> numDistBits)) + 3;
            if (sbOffset + len > (1 << 12))
              return 0;
            UInt32 dist = (v >> (16 - numDistBits));
            if (dist >= sbOffset)
              return 0;
            Int32 offs = -1 - dist;
            Byte *p = dest + destSize;
            for (UInt32 t = 0; t < len; t++)
              p[t] = p[t + offs];
            destSize += len;
            sbOffset += len;
          }
        }
      }
      while (comprSize > 0);
      src += pos;
    }
  }
  return destSize;
}

STDMETHODIMP CInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize != NULL)
    *processedSize = 0;
  if (_virtPos >= Size)
    return (Size == _virtPos) ? S_OK: E_FAIL;
  if (size == 0)
    return S_OK;
  UInt64 rem = Size - _virtPos;
  if (size > rem)
    size = (UInt32)rem;
  if (_virtPos >= InitializedSize)
  {
    memset((Byte *)data, 0, size);
    _virtPos += size;
    *processedSize = size;
    return S_OK;
  }
  rem = InitializedSize - _virtPos;
  if (size > rem)
    size = (UInt32)rem;

  while (_curRem == 0)
  {
    UInt64 cacheTag = _virtPos >> _chunkSizeLog;
    UInt32 cacheIndex = (UInt32)cacheTag & (kNumCacheChunks - 1);
    if (_tags[cacheIndex] == cacheTag)
    {
      UInt32 chunkSize = (UInt32)1 << _chunkSizeLog;
      UInt32 offset = (UInt32)_virtPos & (chunkSize - 1);
      UInt32 cur = MyMin(chunkSize - offset, size);
      memcpy(data, _outBuf + (cacheIndex << _chunkSizeLog) + offset, cur);
      *processedSize = cur;
      _virtPos += cur;
      return S_OK;
    }

    PRF2(printf("\nVirtPos = %6d", _virtPos));
    
    UInt32 comprUnitSize = (UInt32)1 << CompressionUnit;
    UInt64 virtBlock = _virtPos >> BlockSizeLog;
    UInt64 virtBlock2 = virtBlock & ~((UInt64)comprUnitSize - 1);
    
    int left = 0, right = Extents.Size();
    for (;;)
    {
      int mid = (left + right) / 2;
      if (mid == left)
        break;
      if (virtBlock2 < Extents[mid].Virt)
        right = mid;
      else
        left = mid;
    }
    
    bool isCompressed = false;
    UInt64 virtBlock2End = virtBlock2 + comprUnitSize;
    if (CompressionUnit != 0)
      for (int i = left; i < Extents.Size(); i++)
      {
        const CExtent &e = Extents[i];
        if (e.Virt >= virtBlock2End)
          break;
        if (e.IsEmpty())
        {
          isCompressed = true;
          break;
        }
      }

    int i;
    for (i = left; Extents[i + 1].Virt <= virtBlock; i++);
    
    _sparseMode = false;
    if (!isCompressed)
    {
      const CExtent &e = Extents[i];
      UInt64 newPos = (e.Phy << BlockSizeLog) + _virtPos - (e.Virt << BlockSizeLog);
      if (newPos != _physPos)
      {
        _physPos = newPos;
        RINOK(SeekToPhys());
      }
      UInt64 next = Extents[i + 1].Virt;
      if (next > virtBlock2End)
        next &= ~((UInt64)comprUnitSize - 1);
      next <<= BlockSizeLog;
      if (next > Size)
        next = Size;
      _curRem = next - _virtPos;
      break;
    }
    bool thereArePhy = false;
    for (int i2 = left; i2 < Extents.Size(); i2++)
    {
      const CExtent &e = Extents[i2];
      if (e.Virt >= virtBlock2End)
        break;
      if (!e.IsEmpty())
      {
        thereArePhy = true;
        break;
      }
    }
    if (!thereArePhy)
    {
      _curRem = (Extents[i + 1].Virt << BlockSizeLog) - _virtPos;
      _sparseMode = true;
      break;
    }
    
    size_t offs = 0;
    UInt64 curVirt = virtBlock2;
    for (i = left; i < Extents.Size(); i++)
    {
      const CExtent &e = Extents[i];
      if (e.IsEmpty())
        break;
      if (e.Virt >= virtBlock2End)
        return S_FALSE;
      UInt64 newPos = (e.Phy + (curVirt - e.Virt)) << BlockSizeLog;
      if (newPos != _physPos)
      {
        _physPos = newPos;
        RINOK(SeekToPhys());
      }
      UInt64 numChunks = Extents[i + 1].Virt - curVirt;
      if (curVirt + numChunks > virtBlock2End)
        numChunks = virtBlock2End - curVirt;
      size_t compressed = (size_t)numChunks << BlockSizeLog;
      RINOK(ReadStream_FALSE(Stream, _inBuf + offs, compressed));
      curVirt += numChunks;
      _physPos += compressed;
      offs += compressed;
    }
    size_t destLenMax = GetCuSize();
    size_t destLen = destLenMax;
    UInt64 rem = Size - (virtBlock2 << BlockSizeLog);
    if (destLen > rem)
      destLen = (size_t)rem;

    Byte *dest = _outBuf + (cacheIndex << _chunkSizeLog);
    size_t destSizeRes = Lznt1Dec(dest, destLenMax, destLen, _inBuf, offs);
    _tags[cacheIndex] = cacheTag;

    // some files in Vista have destSize > destLen
    if (destSizeRes < destLen)
    {
      memset(dest, 0, destLenMax);
      if (InUse)
        return S_FALSE;
    }
  }
  if (size > _curRem)
    size = (UInt32)_curRem;
  HRESULT res = S_OK;
  if (_sparseMode)
    memset(data, 0, size);
  else
  {
    res = Stream->Read(data, size, &size);
    _physPos += size;
  }
  if (processedSize != NULL)
    *processedSize = size;
  _virtPos += size;
  _curRem -= size;
  return res;
}
 
STDMETHODIMP CInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  UInt64 newVirtPos = offset;
  switch(seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: newVirtPos += _virtPos; break;
    case STREAM_SEEK_END: newVirtPos += Size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (_virtPos != newVirtPos)
    _curRem = 0;
  _virtPos = newVirtPos;
  if (newPosition)
    *newPosition = newVirtPos;
  return S_OK;
}

class CByteBufStream:
  public IInStream,
  public CMyUnknownImp
{
  UInt64 _virtPos;
public:
  CByteBuffer Buf;
  void Init() { _virtPos = 0; }
 
  MY_UNKNOWN_IMP1(IInStream)

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
};

STDMETHODIMP CByteBufStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize != NULL)
    *processedSize = 0;
  if (_virtPos >= Buf.GetCapacity())
    return (_virtPos == Buf.GetCapacity()) ? S_OK: E_FAIL;
  UInt64 rem = Buf.GetCapacity() - _virtPos;
  if (rem < size)
    size = (UInt32)rem;
  memcpy(data, Buf + (size_t)_virtPos, size);
  if (processedSize != NULL)
    *processedSize = size;
  _virtPos += size;
  return S_OK;
}

STDMETHODIMP CByteBufStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  switch(seekOrigin)
  {
    case STREAM_SEEK_SET: _virtPos = offset; break;
    case STREAM_SEEK_CUR: _virtPos += offset; break;
    case STREAM_SEEK_END: _virtPos = Buf.GetCapacity() + offset; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}

static HRESULT DataParseExtents(int clusterSizeLog, const CObjectVector<CAttr> &attrs,
    int attrIndex, int attrIndexLim, UInt64 numPhysClusters, CRecordVector<CExtent> &Extents)
{
  CExtent e;
  e.Virt = 0;
  e.Phy = kEmptyExtent;
  Extents.Add(e);
  const CAttr &attr0 = attrs[attrIndex];

  if (attr0.AllocatedSize < attr0.Size ||
      (attrs[attrIndexLim - 1].HighVcn + 1) != (attr0.AllocatedSize >> clusterSizeLog) ||
      (attr0.AllocatedSize & ((1 << clusterSizeLog) - 1)) != 0)
    return S_FALSE;
  
  for (int i = attrIndex; i < attrIndexLim; i++)
    if (!attrs[i].ParseExtents(Extents, numPhysClusters, attr0.CompressionUnit))
      return S_FALSE;

  UInt64 packSizeCalc = 0;
  for (int k = 0; k < Extents.Size(); k++)
  {
    CExtent &e = Extents[k];
    if (!e.IsEmpty())
      packSizeCalc += (Extents[k + 1].Virt - e.Virt) << clusterSizeLog;
    PRF2(printf("\nSize = %4I64X", Extents[k + 1].Virt - e.Virt));
    PRF2(printf("  Pos = %4I64X", e.Phy));
  }
  
  if (attr0.CompressionUnit != 0)
  {
    if (packSizeCalc != attr0.PackSize)
      return S_FALSE;
  }
  else
  {
    if (packSizeCalc != attr0.AllocatedSize)
      return S_FALSE;
  }
  return S_OK;
}

struct CDataRef
{
  int Start;
  int Num;
};

static const UInt32 kMagic_FILE = 0x454c4946;
static const UInt32 kMagic_BAAD = 0x44414142;

struct CMftRec
{
  UInt32 Magic;
  // UInt64 Lsn;
  UInt16 SeqNumber;
  UInt16 Flags;
  // UInt16 LinkCount;
  // UInt16 NextAttrInstance;
  CMftRef BaseMftRef;
  // UInt32 ThisRecNumber;
  UInt32 MyNumNameLinks;

  CObjectVector<CAttr> DataAttrs;
  CObjectVector<CFileNameAttr> FileNames;
  CRecordVector<CDataRef> DataRefs;

  CSiAttr SiAttr;

  void MoveAttrsFrom(CMftRec &src)
  {
    DataAttrs += src.DataAttrs;
    FileNames += src.FileNames;
    src.DataAttrs.ClearAndFree();
    src.FileNames.ClearAndFree();
  }

  UInt64 GetPackSize() const
  {
    UInt64 res = 0;
    for (int i = 0; i < DataRefs.Size(); i++)
      res += DataAttrs[DataRefs[i].Start].GetPackSize();
    return res;
  }

  bool Parse(Byte *p, int sectorSizeLog, UInt32 numSectors, UInt32 recNumber, CObjectVector<CAttr> *attrs);

  bool IsEmpty() const { return (Magic <= 2); }
  bool IsFILE() const { return (Magic == kMagic_FILE); }
  bool IsBAAD() const { return (Magic == kMagic_BAAD); }

  bool InUse() const { return (Flags & 1) != 0; }
  bool IsDir() const { return (Flags & 2) != 0; }

  void ParseDataNames();
  HRESULT GetStream(IInStream *mainStream, int dataIndex,
      int clusterSizeLog, UInt64 numPhysClusters, IInStream **stream) const;
  int GetNumExtents(int dataIndex, int clusterSizeLog, UInt64 numPhysClusters) const;

  UInt64 GetSize(int dataIndex) const { return DataAttrs[DataRefs[dataIndex].Start].GetSize(); }

  CMftRec(): MyNumNameLinks(0) {}
};

void CMftRec::ParseDataNames()
{
  DataRefs.Clear();
  DataAttrs.Sort(CompareAttr, 0);

  for (int i = 0; i < DataAttrs.Size();)
  {
    CDataRef ref;
    ref.Start = i;
    for (i++; i < DataAttrs.Size(); i++)
      if (DataAttrs[ref.Start].Name != DataAttrs[i].Name)
        break;
    ref.Num = i - ref.Start;
    DataRefs.Add(ref);
  }
}

HRESULT CMftRec::GetStream(IInStream *mainStream, int dataIndex,
    int clusterSizeLog, UInt64 numPhysClusters, IInStream **destStream) const
{
  *destStream = 0;
  CByteBufStream *streamSpec = new CByteBufStream;
  CMyComPtr<IInStream> streamTemp = streamSpec;

  if (dataIndex < 0)
    return E_FAIL;

  if (dataIndex < DataRefs.Size())
  {
    const CDataRef &ref = DataRefs[dataIndex];
    int numNonResident = 0;
    int i;
    for (i = ref.Start; i < ref.Start + ref.Num; i++)
      if (DataAttrs[i].NonResident)
        numNonResident++;

    const CAttr &attr0 = DataAttrs[ref.Start];
      
    if (numNonResident != 0 || ref.Num != 1)
    {
      if (numNonResident != ref.Num || !attr0.IsCompressionUnitSupported())
        return S_FALSE;
      CInStream *streamSpec = new CInStream;
      CMyComPtr<IInStream> streamTemp = streamSpec;
      RINOK(DataParseExtents(clusterSizeLog, DataAttrs, ref.Start, ref.Start + ref.Num, numPhysClusters, streamSpec->Extents));
      streamSpec->Size = attr0.Size;
      streamSpec->InitializedSize = attr0.InitializedSize;
      streamSpec->Stream = mainStream;
      streamSpec->BlockSizeLog = clusterSizeLog;
      streamSpec->InUse = InUse();
      RINOK(streamSpec->InitAndSeek(attr0.CompressionUnit));
      *destStream = streamTemp.Detach();
      return S_OK;
    }
    streamSpec->Buf = attr0.Data;
  }
  streamSpec->Init();
  *destStream = streamTemp.Detach();
  return S_OK;
}

int CMftRec::GetNumExtents(int dataIndex, int clusterSizeLog, UInt64 numPhysClusters) const
{
  if (dataIndex < 0)
    return 0;
  {
    const CDataRef &ref = DataRefs[dataIndex];
    int numNonResident = 0;
    int i;
    for (i = ref.Start; i < ref.Start + ref.Num; i++)
      if (DataAttrs[i].NonResident)
        numNonResident++;

    const CAttr &attr0 = DataAttrs[ref.Start];
      
    if (numNonResident != 0 || ref.Num != 1)
    {
      if (numNonResident != ref.Num || !attr0.IsCompressionUnitSupported())
        return 0; // error;
      CRecordVector<CExtent> extents;
      if (DataParseExtents(clusterSizeLog, DataAttrs, ref.Start, ref.Start + ref.Num, numPhysClusters, extents) != S_OK)
        return 0; // error;
      return extents.Size() - 1;
    }
    // if (attr0.Data.GetCapacity() != 0)
    //   return 1;
    return 0;
  }
}

bool CMftRec::Parse(Byte *p, int sectorSizeLog, UInt32 numSectors, UInt32 recNumber,
    CObjectVector<CAttr> *attrs)
{
  G32(p, Magic);
  if (!IsFILE())
    return IsEmpty() || IsBAAD();

  UInt32 usaOffset;
  UInt32 numUsaItems;
  G16(p + 0x04, usaOffset);
  G16(p + 0x06, numUsaItems);
  
  if ((usaOffset & 1) != 0 || usaOffset + numUsaItems * 2 > ((UInt32)1 << sectorSizeLog) - 2 ||
      numUsaItems == 0 || numUsaItems - 1 != numSectors)
    return false;

  UInt16 usn = Get16(p + usaOffset);
  // PRF(printf("\nusn = %d", usn));
  for (UInt32 i = 1; i < numUsaItems; i++)
  {
    void *pp = p + (i << sectorSizeLog)  - 2;
    if (Get16(pp) != usn)
      return false;
    SetUi16(pp, Get16(p + usaOffset + i * 2));
  }

  // G64(p + 0x08, Lsn);
  G16(p + 0x10, SeqNumber);
  // G16(p + 0x12, LinkCount);
  // PRF(printf(" L=%d", LinkCount));
  UInt32 attrOffs = Get16(p + 0x14);
  G16(p + 0x16, Flags);
  PRF(printf(" F=%4X", Flags));

  UInt32 bytesInUse = Get32(p + 0x18);
  UInt32 bytesAlloc = Get32(p + 0x1C);
  G64(p + 0x20, BaseMftRef.Val);
  if (BaseMftRef.Val != 0)
  {
    PRF(printf("  BaseRef=%d", (int)BaseMftRef.Val));
    // return false; // Check it;
  }
  // G16(p + 0x28, NextAttrInstance);
  if (usaOffset >= 0x30)
    if (Get32(p + 0x2C) != recNumber) // NTFS 3.1+
      return false;

  UInt32 limit = numSectors << sectorSizeLog;
  if (attrOffs >= limit || (attrOffs & 7) != 0 || bytesInUse > limit
      || bytesAlloc != limit)
    return false;


  for (UInt32 t = attrOffs; t < limit;)
  {
    CAttr attr;
    // PRF(printf("\n  %2d:", Attrs.Size()));
    PRF(printf("\n"));
    UInt32 length = attr.Parse(p + t, limit - t);
    if (length == 0 || limit - t < length)
      return false;
    t += length;
    if (attr.Type == 0xFFFFFFFF)
      break;
    switch(attr.Type)
    {
      case ATTR_TYPE_FILE_NAME:
      {
        CFileNameAttr fna;
        if (!attr.ParseFileName(fna))
          return false;
        FileNames.Add(fna);
        PRF(printf("  flags = %4x", (int)fna.NameType));
        PRF(printf("\n  %S", fna.Name));
        break;
      }
      case ATTR_TYPE_STANDARD_INFO:
        if (!attr.ParseSi(SiAttr))
          return false;
        break;
      case ATTR_TYPE_DATA:
        DataAttrs.Add(attr);
        break;
      default:
        if (attrs)
          attrs->Add(attr);
        break;
    }
  }

  return true;
}

struct CItem
{
  int RecIndex;
  int DataIndex;
  CMftRef ParentRef;
  UString Name;
  UInt32 Attrib;

  bool IsDir() const { return (DataIndex < 0); }
};

struct CDatabase
{
  CHeader Header;
  CObjectVector<CItem> Items;
  CObjectVector<CMftRec> Recs;
  CMyComPtr<IInStream> InStream;
  IArchiveOpenCallback *OpenCallback;

  CByteBuffer ByteBuf;

  CObjectVector<CAttr> VolAttrs;

  ~CDatabase() { ClearAndClose(); }

  void Clear();
  void ClearAndClose();

  UString GetItemPath(Int32 index) const;
  HRESULT Open();
  HRESULT ReadDir(Int32 parent, UInt32 cluster, int level);

  HRESULT SeekToCluster(UInt64 cluster);

  int FindMtfRec(const CMftRef &ref) const
  {
    UInt64 val = ref.GetIndex();
    int left = 0, right = Items.Size();
    while (left != right)
    {
      int mid = (left + right) / 2;
      UInt64 midValue = Items[mid].RecIndex;
      if (val == midValue)
        return mid;
      if (val < midValue)
        right = mid;
      else
        left = mid + 1;
    }
    return -1;
  }

};

HRESULT CDatabase::SeekToCluster(UInt64 cluster)
{
  return InStream->Seek(cluster << Header.ClusterSizeLog, STREAM_SEEK_SET, NULL);
}

void CDatabase::Clear()
{
  Items.Clear();
  Recs.Clear();
}

void CDatabase::ClearAndClose()
{
  Clear();
  InStream.Release();
}

#define MY_DIR_PREFIX(x) L"[" x L"]" WSTRING_PATH_SEPARATOR

UString CDatabase::GetItemPath(Int32 index) const
{
  const CItem *item = &Items[index];
  UString name = item->Name;
  for (int j = 0; j < 256; j++)
  {
    CMftRef ref = item->ParentRef;
    index = FindMtfRec(ref);
    if (ref.GetIndex() == 5)
      return name;
    if (index < 0 || Recs[Items[index].RecIndex].SeqNumber != ref.GetNumber())
      return MY_DIR_PREFIX(L"UNKNOWN") + name;
    item = &Items[index];
    name = item->Name + WCHAR_PATH_SEPARATOR + name;
  }
  return MY_DIR_PREFIX(L"BAD") + name;
}

HRESULT CDatabase::Open()
{
  Clear();
  
  static const UInt32 kHeaderSize = 512;
  Byte buf[kHeaderSize];
  RINOK(ReadStream_FALSE(InStream, buf, kHeaderSize));
  if (!Header.Parse(buf))
    return S_FALSE;
  UInt64 fileSize;
  RINOK(InStream->Seek(0, STREAM_SEEK_END, &fileSize));
  if (fileSize < Header.GetPhySize())
    return S_FALSE;
  
  SeekToCluster(Header.MftCluster);

  CMftRec mftRec;
  UInt32 numSectorsInRec;
  int recSizeLog;
  CMyComPtr<IInStream> mftStream;
  {
    UInt32 blockSize = 1 << 12;
    ByteBuf.SetCapacity(blockSize);
    RINOK(ReadStream_FALSE(InStream, ByteBuf, blockSize));
    
    UInt32 allocSize = Get32(ByteBuf + 0x1C);
    recSizeLog = GetLog(allocSize);
    if (recSizeLog < Header.SectorSizeLog)
      return false;
    numSectorsInRec = 1 << (recSizeLog - Header.SectorSizeLog);
    if (!mftRec.Parse(ByteBuf, Header.SectorSizeLog, numSectorsInRec, NULL, 0))
      return S_FALSE;
    if (!mftRec.IsFILE())
      return S_FALSE;
    mftRec.ParseDataNames();
    if (mftRec.DataRefs.IsEmpty())
      return S_FALSE;
    RINOK(mftRec.GetStream(InStream, 0, Header.ClusterSizeLog, Header.NumClusters, &mftStream));
    if (!mftStream)
      return S_FALSE;
  }

  UInt64 mftSize = mftRec.DataAttrs[0].Size;
  if ((mftSize >> 4) > Header.GetPhySize())
    return S_FALSE;

  UInt64 numFiles = mftSize >> recSizeLog;
  if (numFiles > (1 << 30))
    return S_FALSE;
  if (OpenCallback)
  {
    RINOK(OpenCallback->SetTotal(&numFiles, &mftSize));
  }
  const UInt32 kBufSize = (1 << 15);
  if (kBufSize < (1 << recSizeLog))
    return S_FALSE;

  ByteBuf.SetCapacity((size_t)kBufSize);
  Recs.Reserve((int)numFiles);
  for (UInt64 pos64 = 0;;)
  {
    if (OpenCallback)
    {
      UInt64 numFiles = Recs.Size();
      if ((numFiles & 0x3FF) == 0)
      {
        RINOK(OpenCallback->SetCompleted(&numFiles, &pos64));
      }
    }
    UInt32 readSize = kBufSize;
    UInt64 rem = mftSize - pos64;
    if (readSize > rem)
      readSize = (UInt32)rem;
    if (readSize < ((UInt32)1 << recSizeLog))
      break;
    RINOK(ReadStream_FALSE(mftStream, ByteBuf, (size_t)readSize));
    pos64 += readSize;
    for (int i = 0; ((UInt32)(i + 1) << recSizeLog) <= readSize; i++)
    {
      PRF(printf("\n---------------------"));
      PRF(printf("\n%5d:", Recs.Size()));
      Byte *p = ByteBuf + ((UInt32)i << recSizeLog);
      CMftRec rec;
      if (!rec.Parse(p, Header.SectorSizeLog, numSectorsInRec, (UInt32)Recs.Size(),
          (Recs.Size() == kRecIndex_Volume) ? &VolAttrs: NULL))
        return S_FALSE;
      Recs.Add(rec);
    }
  }

  int i;
  for (i = 0; i < Recs.Size(); i++)
  {
    CMftRec &rec = Recs[i];
    if (!rec.BaseMftRef.IsBaseItself())
    {
      UInt64 refIndex = rec.BaseMftRef.GetIndex();
      if (refIndex > (UInt32)Recs.Size())
        return S_FALSE;
      CMftRec &refRec = Recs[(int)refIndex];
      bool moveAttrs = (refRec.SeqNumber == rec.BaseMftRef.GetNumber() && refRec.BaseMftRef.IsBaseItself());
      if (rec.InUse() && refRec.InUse())
      {
        if (!moveAttrs)
          return S_FALSE;
      }
      else if (rec.InUse() || refRec.InUse())
        moveAttrs = false;
      if (moveAttrs)
        refRec.MoveAttrsFrom(rec);
    }
  }

  for (i = 0; i < Recs.Size(); i++)
    Recs[i].ParseDataNames();
  
  for (i = 0; i < Recs.Size(); i++)
  {
    CMftRec &rec = Recs[i];
    if (!rec.IsFILE() || !rec.BaseMftRef.IsBaseItself())
      continue;
    int numNames = 0;
    // printf("\n%4d: ", i);
    for (int t = 0; t < rec.FileNames.Size(); t++)
    {
      const CFileNameAttr &fna = rec.FileNames[t];
      // printf("%4d %S  | ", (int)fna.NameType, fna.Name);
      if (fna.IsDos())
        continue;
      int numDatas = rec.DataRefs.Size();

      // For hard linked files we show substreams only for first Name.
      if (numDatas > 1 && numNames > 0)
        numDatas = 1;
      numNames++;

      if (rec.IsDir())
      {
        CItem item;
        item.Name = fna.Name;
        item.RecIndex = i;
        item.DataIndex = -1;
        item.ParentRef = fna.ParentDirRef;
        item.Attrib = rec.SiAttr.Attrib | 0x10;
        // item.Attrib = fna.Attrib;
        Items.Add(item);
      }
      for (int di = 0; di < numDatas; di++)
      {
        CItem item;
        item.Name = fna.Name;
        item.Attrib = rec.SiAttr.Attrib;
        const UString &subName = rec.DataAttrs[rec.DataRefs[di].Start].Name;
        if (!subName.IsEmpty())
        {
          // $BadClus:$Bad is sparse file for all clusters. So we skip it.
          if (i == kRecIndex_BadClus && subName == L"$Bad")
            continue;
          item.Name += L":";
          item.Name += subName;
          item.Attrib = fna.Attrib;
        }
        
        PRF(printf("\n%3d", i));
        PRF(printf("  attrib=%2x", rec.SiAttr.Attrib));
        PRF(printf(" %S", item.Name));
        
        item.RecIndex = i;
        item.DataIndex = di;
        item.ParentRef = fna.ParentDirRef;

        Items.Add(item);
        rec.MyNumNameLinks++;
      }
    }
    rec.FileNames.ClearAndFree();
  }
  
  return S_OK;
}

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp,
  CDatabase
{
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  IInStream *stream2;
  const CItem &item = Items[index];
  const CMftRec &rec = Recs[item.RecIndex];
  HRESULT res = rec.GetStream(InStream, item.DataIndex, Header.ClusterSizeLog, Header.NumClusters, &stream2);
  *stream = (ISequentialInStream *)stream2;
  return res;
  COM_TRY_END
}

static const STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidATime, VT_FILETIME},
  { NULL, kpidAttrib, VT_UI4},
  { NULL, kpidLinks, VT_UI4},
  { NULL, kpidNumBlocks, VT_UI4}
};

static const STATPROPSTG kArcProps[] =
{
  { NULL, kpidVolumeName, VT_BSTR},
  { NULL, kpidFileSystem, VT_BSTR},
  { NULL, kpidClusterSize, VT_UI4},
  { NULL, kpidPhySize, VT_UI8},
  { NULL, kpidHeadersSize, VT_UI8},
  { NULL, kpidCTime, VT_FILETIME},

  { NULL, kpidSectorSize, VT_UI4},
  { NULL, kpidId, VT_UI8}
  // { NULL, kpidSectorsPerTrack, VT_UI4},
  // { NULL, kpidNumHeads, VT_UI4},
  // { NULL, kpidHiddenSectors, VT_UI4}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static void NtfsTimeToProp(UInt64 t, NWindows::NCOM::CPropVariant &prop)
{
  FILETIME ft;
  ft.dwLowDateTime = (DWORD)t;
  ft.dwHighDateTime = (DWORD)(t >> 32);
  prop = ft;
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;

  const CMftRec *volRec = (Recs.Size() > kRecIndex_Volume ? &Recs[kRecIndex_Volume] : NULL);

  switch(propID)
  {
    case kpidClusterSize: prop = Header.ClusterSize(); break;
    case kpidPhySize: prop = Header.GetPhySize(); break;
    /*
    case kpidHeadersSize:
    {
      UInt64 val = 0;
      for (int i = 0; i < kNumSysRecs; i++)
      {
        printf("\n%2d: %8I64d ", i, Recs[i].GetPackSize());
        if (i == 8)
          i = i
        val += Recs[i].GetPackSize();
      }
      prop = val;
      break;
    }
    */
    case kpidCTime: if (volRec) NtfsTimeToProp(volRec->SiAttr.CTime, prop); break;break;
    case kpidVolumeName:
    {
      for (int i = 0; i < VolAttrs.Size(); i++)
      {
        const CAttr &attr = VolAttrs[i];
        if (attr.Type == ATTR_TYPE_VOLUME_NAME)
        {
          UString name;
          GetString(attr.Data, (int)attr.Data.GetCapacity() / 2, name);
          prop = name;
          break;
        }
      }
      break;
    }
    case kpidFileSystem:
    {
      AString s = "NTFS";
      for (int i = 0; i < VolAttrs.Size(); i++)
      {
        const CAttr &attr = VolAttrs[i];
        if (attr.Type == ATTR_TYPE_VOLUME_INFO)
        {
          CVolInfo vi;
          if (attr.ParseVolInfo(vi))
          {
            s += ' ';
            char temp[16];
            ConvertUInt32ToString(vi.MajorVer, temp);
            s += temp;
            s += '.';
            ConvertUInt32ToString(vi.MinorVer, temp);
            s += temp;
          }
          break;
        }
      }
      prop = s;
      break;
    }
    case kpidSectorSize: prop = (UInt32)1 << Header.SectorSizeLog; break;
    case kpidId: prop = Header.SerialNumber; break;
    // case kpidMediaType: prop = Header.MediaType; break;
    // case kpidSectorsPerTrack: prop = Header.SectorsPerTrack; break;
    // case kpidNumHeads: prop = Header.NumHeads; break;
    // case kpidHiddenSectors: prop = Header.NumHiddenSectors; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItem &item = Items[index];
  const CMftRec &rec = Recs[item.RecIndex];

  const CAttr *data= NULL;
  if (item.DataIndex >= 0)
    data = &rec.DataAttrs[rec.DataRefs[item.DataIndex].Start];

  switch(propID)
  {
    case kpidPath:
    {
      UString name = GetItemPath(index);
      const wchar_t *prefix = NULL;
      if (!rec.InUse())
        prefix = MY_DIR_PREFIX(L"DELETED");
      else if (item.RecIndex < kNumSysRecs)
        prefix = MY_DIR_PREFIX(L"SYSTEM");
      if (prefix)
        name = prefix + name;
      prop = name;
      break;
    }

    case kpidIsDir: prop = item.IsDir(); break;
    case kpidMTime: NtfsTimeToProp(rec.SiAttr.MTime, prop); break;
      
    case kpidCTime: NtfsTimeToProp(rec.SiAttr.CTime, prop); break;
    case kpidATime: NtfsTimeToProp(rec.SiAttr.ATime, prop); break;
    case kpidAttrib:
      prop = item.Attrib;
      break;
    case kpidLinks: prop = rec.MyNumNameLinks; break;
    case kpidSize: if (data) prop = data->GetSize(); break;
    case kpidPackSize: if (data) prop = data->GetPackSize(); break;
    case kpidNumBlocks: if (data) prop = (UInt32)rec.GetNumExtents(item.DataIndex, Header.ClusterSizeLog, Header.NumClusters); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  {
    OpenCallback = callback;
    InStream = stream;
    HRESULT res;
    try
    {
      res = CDatabase::Open();
      if (res == S_OK)
        return S_OK;
    }
    catch(...)
    {
      Close();
      throw;
    }
    Close();
    return res;
  }
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  ClearAndClose();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = Items.Size();
  if (numItems == 0)
    return S_OK;
  UInt32 i;
  UInt64 totalSize = 0;
  for (i = 0; i < numItems; i++)
  {
    const CItem &item = Items[allFilesMode ? i : indices[i]];
    const CMftRec &rec = Recs[item.RecIndex];
    if (!rec.IsDir())
      totalSize += rec.GetSize(item.DataIndex);
  }
  RINOK(extractCallback->SetTotal(totalSize));

  UInt64 totalPackSize;
  totalSize = totalPackSize = 0;
  
  CByteBuffer buf;
  UInt32 clusterSize = Header.ClusterSize();
  buf.SetCapacity(clusterSize);

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);

  for (i = 0; i < numItems; i++)
  {
    lps->InSize = totalPackSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    const CItem &item = Items[index];
    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }

    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));

    outStreamSpec->SetStream(realOutStream);
    realOutStream.Release();
    outStreamSpec->Init();

    const CMftRec &rec = Recs[item.RecIndex];
    const CAttr &data = rec.DataAttrs[rec.DataRefs[item.DataIndex].Start];

    int res = NExtract::NOperationResult::kDataError;
    {
      CMyComPtr<IInStream> inStream;
      HRESULT hres = rec.GetStream(InStream, item.DataIndex, Header.ClusterSizeLog, Header.NumClusters, &inStream);
      if (hres == S_FALSE)
        res = NExtract::NOperationResult::kUnSupportedMethod;
      else
      {
        RINOK(hres);
        if (inStream)
        {
          HRESULT hres = copyCoder->Code(inStream, outStream, NULL, NULL, progress);
          if (hres != S_OK &&  hres != S_FALSE)
          {
            RINOK(hres);
          }
          if (/* copyCoderSpec->TotalSize == item.GetSize() && */ hres == S_OK)
            res = NExtract::NOperationResult::kOK;
        }
      }
    }
    totalPackSize += data.GetPackSize();
    totalSize += data.GetSize();
    outStreamSpec->ReleaseStream();
    RINOK(extractCallback->SetOperationResult(res));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = Items.Size();
  return S_OK;
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"NTFS", L"ntfs img", 0, 0xD9, { 'N', 'T', 'F', 'S', ' ', ' ', ' ', ' ', 0 }, 9, false, CreateArc, 0 };

REGISTER_ARC(Fat)

}}
