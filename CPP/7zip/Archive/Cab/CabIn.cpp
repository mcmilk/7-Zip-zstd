// Archive/CabIn.cpp

#include "StdAfx.h"

#include "../Common/FindSignature.h"

#include "CabIn.h"

namespace NArchive {
namespace NCab {

Byte CInArchive::Read8()
{
  Byte b;
  if (!inBuffer.ReadByte(b))
    throw CInArchiveException(CInArchiveException::kUnsupported);
  return b;
}

UInt16 CInArchive::Read16()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = Read8();
    value |= (UInt16(b) << (8 * i));
  }
  return value;
}

UInt32 CInArchive::Read32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b = Read8();
    value |= (UInt32(b) << (8 * i));
  }
  return value;
}

AString CInArchive::SafeReadName()
{
  AString name;
  for (;;)
  {
    Byte b = Read8();
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

void CInArchive::Skip(UInt32 size)
{
  while (size-- != 0)
    Read8();
}

HRESULT CInArchive::Open(const UInt64 *searchHeaderSizeLimit, CDatabaseEx &db)
{
  IInStream *stream = db.Stream;
  db.Clear();
  RINOK(stream->Seek(0, STREAM_SEEK_SET, &db.StartPosition));

  RINOK(FindSignatureInStream(stream, NHeader::kMarker, NHeader::kMarkerSize,
      searchHeaderSizeLimit, db.StartPosition));

  RINOK(stream->Seek(db.StartPosition + NHeader::kMarkerSize, STREAM_SEEK_SET, NULL));
  if (!inBuffer.Create(1 << 17))
    return E_OUTOFMEMORY;
  inBuffer.SetStream(stream);
  inBuffer.Init();

  CInArchiveInfo &ai = db.ArchiveInfo;

  ai.Size = Read32();
  if (Read32() != 0)
    return S_FALSE;
  ai.FileHeadersOffset = Read32();
  if (Read32() != 0)
    return S_FALSE;

  ai.VersionMinor = Read8();
  ai.VersionMajor = Read8();
  ai.NumFolders = Read16();
  ai.NumFiles = Read16();
  ai.Flags = Read16();
  if (ai.Flags > 7)
    return S_FALSE;
  ai.SetID = Read16();
  ai.CabinetNumber = Read16();

  if (ai.ReserveBlockPresent())
  {
    ai.PerCabinetAreaSize = Read16();
    ai.PerFolderAreaSize = Read8();
    ai.PerDataBlockAreaSize = Read8();

    Skip(ai.PerCabinetAreaSize);
  }

  {
    if (ai.IsTherePrev())
      ReadOtherArchive(ai.PrevArc);
    if (ai.IsThereNext())
      ReadOtherArchive(ai.NextArc);
  }
  
  int i;
  for (i = 0; i < ai.NumFolders; i++)
  {
    CFolder folder;

    folder.DataStart = Read32();
    folder.NumDataBlocks = Read16();
    folder.CompressionTypeMajor = Read8();
    folder.CompressionTypeMinor = Read8();

    Skip(ai.PerFolderAreaSize);
    db.Folders.Add(folder);
  }
  
  RINOK(stream->Seek(db.StartPosition + ai.FileHeadersOffset, STREAM_SEEK_SET, NULL));

  inBuffer.SetStream(stream);
  inBuffer.Init();
  for (i = 0; i < ai.NumFiles; i++)
  {
    CItem item;
    item.Size = Read32();
    item.Offset = Read32();
    item.FolderIndex = Read16();
    UInt16 pureDate = Read16();
    UInt16 pureTime = Read16();
    item.Time = ((UInt32(pureDate) << 16)) | pureTime;
    item.Attributes = Read16();
    item.Name = SafeReadName();
    int folderIndex = item.GetFolderIndex(db.Folders.Size());
    if (folderIndex >= db.Folders.Size())
      return S_FALSE;
    db.Items.Add(item);
  }
  return S_OK;
}

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

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
  RINOZ(MyCompare(p1->VolumeIndex, p2->VolumeIndex));
  return MyCompare(p1->ItemIndex, p2->ItemIndex);
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
    int folderIndex = GetFolderIndex(&Items[i]);
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
  UInt32 beginPos = 0;
  UInt64 endPos = 0;
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
      prevFolder = folderIndex;
    else if (item.Offset < endPos &&
        (item.Offset != beginPos || item.GetEndOffset() != endPos))
      return false;
    beginPos = item.Offset;
    endPos = item.GetEndOffset();
  }
  return true;
}

}}
