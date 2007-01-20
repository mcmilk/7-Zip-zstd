// Archive/CabIn.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/MyCom.h"
#include "CabIn.h"
#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"

namespace NArchive{
namespace NCab{

/*
static HRESULT ReadBytes(IInStream *inStream, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  RINOK(ReadStream(inStream, data, size, &realProcessedSize));
  if(realProcessedSize != size)
    return S_FALSE;
  return S_OK;
}

static HRESULT SafeRead(IInStream *inStream, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  RINOK(ReadStream(inStream, data, size, &realProcessedSize));
  if(realProcessedSize != size)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
  return S_OK;
}

static void SafeInByteRead(::CInBuffer &inBuffer, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  inBuffer.ReadBytes(data, size, realProcessedSize);
  if(realProcessedSize != size)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
}
*/

Byte CInArchive::ReadByte()
{
  Byte b;
  if (!inBuffer.ReadByte(b))
    throw CInArchiveException(CInArchiveException::kUnsupported);
  return b;
}

UInt16 CInArchive::ReadUInt16()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = ReadByte();
    value |= (UInt16(b) << (8 * i));
  }
  return value;
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b = ReadByte();
    value |= (UInt32(b) << (8 * i));
  }
  return value;
}

AString CInArchive::SafeReadName()
{
  AString name;
  for (;;)
  {
    Byte b = ReadByte();
    if (b == 0)
      return name;
    name += (char)b;
  }
}

void CInArchive::ReadOtherArchive(COtherArchive &oa)
{
  oa.FileName = SafeReadName();
  oa.DiskName = SafeReadName();
}

void CInArchive::Skeep(size_t size)
{
  while (size-- != 0)
    ReadByte();
}

HRESULT CInArchive::Open2(IInStream *inStream, 
    const UInt64 *searchHeaderSizeLimit,
    CDatabase &database)
{
  database.Clear();
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &database.StartPosition));

  {
    if (!inBuffer.Create(1 << 17))
      return E_OUTOFMEMORY;
    inBuffer.SetStream(inStream);
    inBuffer.Init();
    UInt64 value = 0;
    const int kSignatureSize = 8;
    UInt64 kSignature64 = NHeader::NArchive::kSignature;
    for (;;)
    {
      Byte b;
      if (!inBuffer.ReadByte(b))
        return S_FALSE;
      value >>= 8;
      value |= ((UInt64)b) << ((kSignatureSize - 1) * 8);
      if (inBuffer.GetProcessedSize() >= kSignatureSize)
      {
        if (value == kSignature64)
          break;
        if (searchHeaderSizeLimit != NULL)
          if (inBuffer.GetProcessedSize() > (*searchHeaderSizeLimit))
            return S_FALSE;
      }
    }
    database.StartPosition += inBuffer.GetProcessedSize() - kSignatureSize;
  }

  CInArchiveInfo &archiveInfo = database.ArchiveInfo;

  archiveInfo.Size = ReadUInt32(); // size of this cabinet file in bytes
  if (ReadUInt32() != 0)
    return S_FALSE;
  archiveInfo.FileHeadersOffset = ReadUInt32(); // offset of the first CFFILE entry
  if (ReadUInt32() != 0)
    return S_FALSE;

  archiveInfo.VersionMinor = ReadByte(); // cabinet file format version, minor
  archiveInfo.VersionMajor = ReadByte(); // cabinet file format version, major
  archiveInfo.NumFolders = ReadUInt16(); // number of CFFOLDER entries in this cabinet
  archiveInfo.NumFiles  = ReadUInt16(); // number of CFFILE entries in this cabinet
  archiveInfo.Flags = ReadUInt16(); // number of CFFILE entries in this cabinet
  archiveInfo.SetID = ReadUInt16(); // must be the same for all cabinets in a set
  archiveInfo.CabinetNumber = ReadUInt16(); // number of this cabinet file in a set

  if (archiveInfo.ReserveBlockPresent())
  {
    archiveInfo.PerCabinetAreaSize = ReadUInt16(); // (optional) size of per-cabinet reserved area
    archiveInfo.PerFolderAreaSize = ReadByte(); // (optional) size of per-folder reserved area
    archiveInfo.PerDataBlockAreaSize = ReadByte(); // (optional) size of per-datablock reserved area

    Skeep(archiveInfo.PerCabinetAreaSize);
  }

  {
    if (archiveInfo.IsTherePrev())
      ReadOtherArchive(archiveInfo.PreviousArchive);
    if (archiveInfo.IsThereNext())
      ReadOtherArchive(archiveInfo.NextArchive);
  }
  
  int i;
  for(i = 0; i < archiveInfo.NumFolders; i++)
  {
    CFolder folder;

    folder.DataStart = ReadUInt32();
    folder.NumDataBlocks = ReadUInt16();
    folder.CompressionTypeMajor = ReadByte();
    folder.CompressionTypeMinor = ReadByte();

    Skeep(archiveInfo.PerFolderAreaSize);
    database.Folders.Add(folder);
  }
  
  RINOK(inStream->Seek(database.StartPosition + archiveInfo.FileHeadersOffset, STREAM_SEEK_SET, NULL));

  inBuffer.SetStream(inStream);
  inBuffer.Init();
  for(i = 0; i < archiveInfo.NumFiles; i++)
  {
    CItem item;
    item.Size = ReadUInt32();
    item.Offset = ReadUInt32();
    item.FolderIndex = ReadUInt16();
    UInt16 pureDate = ReadUInt16();
    UInt16 pureTime = ReadUInt16();
    item.Time = ((UInt32(pureDate) << 16)) | pureTime;
    item.Attributes = ReadUInt16();
    item.Name = SafeReadName();
    int folderIndex = item.GetFolderIndex(database.Folders.Size());
    if (folderIndex >= database.Folders.Size())
      return S_FALSE;
    database.Items.Add(item);
  }
  return S_OK;
}

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

HRESULT CInArchive::Open(
    const UInt64 *searchHeaderSizeLimit,
    CDatabaseEx &database)
{
  return Open2(database.Stream, searchHeaderSizeLimit, database);
}


static int CompareMvItems2(const CMvItem *p1, const CMvItem *p2)
{
  RINOZ(MyCompare(p1->VolumeIndex, p2->VolumeIndex));
  return MyCompare(p1->ItemIndex, p2->ItemIndex);
}

static int CompareMvItems(const CMvItem *p1, const CMvItem *p2, void *param)
{
  const CMvDatabaseEx &mvDb = *(const CMvDatabaseEx *)param;
  const CDatabaseEx &db1 = mvDb.Volumes[p1->VolumeIndex];
  const CDatabaseEx &db2 = mvDb.Volumes[p2->VolumeIndex];
  const CItem &item1 = db1.Items[p1->ItemIndex];
  const CItem &item2 = db2.Items[p2->ItemIndex];;
  bool isDir1 = item1.IsDirectory();
  bool isDir2 = item2.IsDirectory();
  if (isDir1 && !isDir2)
    return -1;
  if (isDir2 && !isDir1)
    return 1;
  int f1 = mvDb.GetFolderIndex(p1);
  int f2 = mvDb.GetFolderIndex(p2);
  RINOZ(MyCompare(f1, f2));
  RINOZ(MyCompare(item1.Offset, item2.Offset));
  RINOZ(MyCompare(item1.Size, item2.Size));
  return CompareMvItems2(p1, p2);
}

bool CMvDatabaseEx::AreItemsEqual(int i1, int i2)
{
  const CMvItem *p1 = &Items[i1];
  const CMvItem *p2 = &Items[i2];
  const CDatabaseEx &db1 = Volumes[p1->VolumeIndex];
  const CDatabaseEx &db2 = Volumes[p2->VolumeIndex];
  const CItem &item1 = db1.Items[p1->ItemIndex];
  const CItem &item2 = db2.Items[p2->ItemIndex];;
  int f1 = GetFolderIndex(p1);
  int f2 = GetFolderIndex(p2);
  if (f1 != f2)
    return false;
  if (item1.Offset != item2.Offset)
    return false;
  if (item1.Size != item2.Size)
    return false;

  return true;
}

void CMvDatabaseEx::FillSortAndShrink()
{
  Items.Clear();
  StartFolderOfVol.Clear();
  FolderStartFileIndex.Clear();
  int offset = 0;
  for (int v = 0; v < Volumes.Size(); v++)
  {
    const CDatabaseEx &db = Volumes[v];
    int curOffset = offset;
    if (db.IsTherePrevFolder())
      curOffset--;
    StartFolderOfVol.Add(curOffset);
    offset += db.GetNumberOfNewFolders();

    CMvItem mvItem;
    mvItem.VolumeIndex = v;
    for (int i = 0 ; i < db.Items.Size(); i++)
    {
      mvItem.ItemIndex = i;
      Items.Add(mvItem);
    }
  }

  Items.Sort(CompareMvItems, (void *)this);
  int j = 1;
  int i;
  for (i = 1; i < Items.Size(); i++)
    if (!AreItemsEqual(i, i -1))
      Items[j++] = Items[i];
  Items.DeleteFrom(j);

  for (i = 0; i < Items.Size(); i++)
  {
    const CMvItem &mvItem = Items[i];
    int folderIndex = GetFolderIndex(&mvItem);
    if (folderIndex >= FolderStartFileIndex.Size())
      FolderStartFileIndex.Add(i);
  }
}

bool CMvDatabaseEx::Check()
{
  for (int v = 1; v < Volumes.Size(); v++)
  {
    const CDatabaseEx &db1 = Volumes[v];
    if (db1.IsTherePrevFolder())
    {
      const CDatabaseEx &db0 = Volumes[v - 1];
      if (db0.Folders.IsEmpty() || db1.Folders.IsEmpty())
        return false;
      const CFolder &f0 = db0.Folders.Back();
      const CFolder &f1 = db1.Folders.Front();
      if (f0.CompressionTypeMajor != f1.CompressionTypeMajor ||
          f0.CompressionTypeMinor != f1.CompressionTypeMinor)
        return false;
    }
  }
  UInt64 maxPos = 0;
  int prevFolder = -2;
  for(int i = 0; i < Items.Size(); i++)
  {
    const CMvItem &mvItem = Items[i];
    int fIndex = GetFolderIndex(&mvItem);
    if (fIndex >= FolderStartFileIndex.Size())
      return false;
    const CItem &item = Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
    if (item.IsDirectory())
      continue;
    int folderIndex = GetFolderIndex(&mvItem);
    if (folderIndex != prevFolder)
    {
      prevFolder = folderIndex;
      maxPos = 0;
      continue;
    }
    if (item.Offset < maxPos)
      return false;
    maxPos = item.GetEndOffset();
    if (maxPos < item.Offset)
      return false;
  }
  return true;
}

}}
