// Archive/CabIn.cpp

#include "StdAfx.h"

#include "CabIn.h"

#include "../Common/FindSignature.h"

namespace NArchive {
namespace NCab {

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

HRESULT CInArchive::Open2(IInStream *stream,
    const UInt64 *searchHeaderSizeLimit,
    CDatabase &database)
{
  database.Clear();
  RINOK(stream->Seek(0, STREAM_SEEK_SET, &database.StartPosition));

  RINOK(FindSignatureInStream(stream, NHeader::kMarker, NHeader::kMarkerSize,
      searchHeaderSizeLimit, database.StartPosition));

  RINOK(stream->Seek(database.StartPosition + NHeader::kMarkerSize, STREAM_SEEK_SET, NULL));
  if (!inBuffer.Create(1 << 17))
    return E_OUTOFMEMORY;
  inBuffer.SetStream(stream);
  inBuffer.Init();

  CInArchiveInfo &ai = database.ArchiveInfo;

  ai.Size = ReadUInt32();
  if (ReadUInt32() != 0)
    return S_FALSE;
  ai.FileHeadersOffset = ReadUInt32();
  if (ReadUInt32() != 0)
    return S_FALSE;

  ai.VersionMinor = ReadByte();
  ai.VersionMajor = ReadByte();
  ai.NumFolders = ReadUInt16();
  ai.NumFiles = ReadUInt16();
  ai.Flags = ReadUInt16();
  if (ai.Flags > 7)
    return S_FALSE;
  ai.SetID = ReadUInt16();
  ai.CabinetNumber = ReadUInt16();

  if (ai.ReserveBlockPresent())
  {
    ai.PerCabinetAreaSize = ReadUInt16();
    ai.PerFolderAreaSize = ReadByte();
    ai.PerDataBlockAreaSize = ReadByte();

    Skeep(ai.PerCabinetAreaSize);
  }

  {
    if (ai.IsTherePrev())
      ReadOtherArchive(ai.PreviousArchive);
    if (ai.IsThereNext())
      ReadOtherArchive(ai.NextArchive);
  }
  
  int i;
  for (i = 0; i < ai.NumFolders; i++)
  {
    CFolder folder;

    folder.DataStart = ReadUInt32();
    folder.NumDataBlocks = ReadUInt16();
    folder.CompressionTypeMajor = ReadByte();
    folder.CompressionTypeMinor = ReadByte();

    Skeep(ai.PerFolderAreaSize);
    database.Folders.Add(folder);
  }
  
  RINOK(stream->Seek(database.StartPosition + ai.FileHeadersOffset, STREAM_SEEK_SET, NULL));

  inBuffer.SetStream(stream);
  inBuffer.Init();
  for (i = 0; i < ai.NumFiles; i++)
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
  bool isDir1 = item1.IsDir();
  bool isDir2 = item2.IsDir();
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
  return GetFolderIndex(p1) == GetFolderIndex(p2) &&
    item1.Offset == item2.Offset &&
    item1.Size == item2.Size &&
    item1.Name == item2.Name;
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
  for (int i = 0; i < Items.Size(); i++)
  {
    const CMvItem &mvItem = Items[i];
    int fIndex = GetFolderIndex(&mvItem);
    if (fIndex >= FolderStartFileIndex.Size())
      return false;
    const CItem &item = Volumes[mvItem.VolumeIndex].Items[mvItem.ItemIndex];
    if (item.IsDir())
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
