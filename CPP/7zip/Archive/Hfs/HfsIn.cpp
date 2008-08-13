// HfsIn.cpp

#include "StdAfx.h"

#include "../../Common/StreamUtils.h"
#include "Common/IntToString.h"

#include "HfsIn.h"

#include "../../../../C/CpuArch.h"

#define Get16(p) GetBe16(p)
#define Get32(p) GetBe32(p)
#define Get64(p) GetBe64(p)

namespace NArchive {
namespace NHfs {

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

static int CompareIdToIndex(const CIdIndexPair *p1, const CIdIndexPair *p2, void * /* param */)
{
  RINOZ(MyCompare(p1->ID, p2->ID));
  return MyCompare(p1->Index, p2->Index);
}

bool operator< (const CIdIndexPair &a1, const CIdIndexPair &a2) { return (a1.ID  < a2.ID); }
bool operator> (const CIdIndexPair &a1, const CIdIndexPair &a2) { return (a1.ID  > a2.ID); }
bool operator==(const CIdIndexPair &a1, const CIdIndexPair &a2) { return (a1.ID == a2.ID); }
bool operator!=(const CIdIndexPair &a1, const CIdIndexPair &a2) { return (a1.ID != a2.ID); }

static UString GetSpecName(const UString &name, UInt32 /* id */)
{
  UString name2 = name;
  name2.Trim();
  if (name2.IsEmpty())
  {
    /*
    wchar_t s[32];
    ConvertUInt64ToString(id, s);
    return L"[" + (UString)s + L"]";
    */
    return L"[]";
  }
  return name;
}

UString CDatabase::GetItemPath(int index) const
{
  const CItem *item = &Items[index];
  UString name = GetSpecName(item->Name, item->ID);

  for (int i = 0; i < 1000; i++)
  {
    if (item->ParentID < 16 && item->ParentID != 2)
    {
      if (item->ParentID != 1)
        break;
      return name;
    }
    CIdIndexPair pair;
    pair.ID = item->ParentID;
    pair.Index = 0;
    int indexInMap = IdToIndexMap.FindInSorted(pair);
    if (indexInMap < 0)
      break;
    item = &Items[IdToIndexMap[indexInMap].Index];
    name = GetSpecName(item->Name, item->ID) + WCHAR_PATH_SEPARATOR + name;
  }
  return (UString)L"Unknown" + WCHAR_PATH_SEPARATOR + name;
}

void CFork::Parse(const Byte *p)
{
  Size = Get64(p);
  // ClumpSize = Get32(p + 8);
  NumBlocks = Get32(p + 0xC);
  for (int i = 0; i < 8; i++)
  {
    CExtent &e = Extents[i];
    e.Pos = Get32(p + 0x10 + i * 8);
    e.NumBlocks = Get32(p + 0x10 + i * 8 + 4);
  }
}

static HRESULT ReadExtent(int blockSizeLog, IInStream *inStream, Byte *buf, const CExtent &e)
{
  RINOK(inStream->Seek((UInt64)e.Pos << blockSizeLog, STREAM_SEEK_SET, NULL));
  return ReadStream_FALSE(inStream, buf, (size_t)e.NumBlocks << blockSizeLog);
}

HRESULT CDatabase::ReadFile(const CFork &fork, CByteBuffer &buf, IInStream *inStream)
{
  if (fork.NumBlocks >= Header.NumBlocks)
    return S_FALSE;
  size_t totalSize = (size_t)fork.NumBlocks << Header.BlockSizeLog;
  if ((totalSize >> Header.BlockSizeLog) != fork.NumBlocks)
    return S_FALSE;
  buf.SetCapacity(totalSize);
  UInt32 curBlock = 0;
  for (int i = 0; i < 8; i++)
  {
    if (curBlock >= fork.NumBlocks)
      break;
    const CExtent &e = fork.Extents[i];
    if (fork.NumBlocks - curBlock < e.NumBlocks || e.Pos >= Header.NumBlocks)
      return S_FALSE;
    RINOK(ReadExtent(Header.BlockSizeLog, inStream,
        (Byte *)buf + ((size_t)curBlock << Header.BlockSizeLog), e));
    curBlock += e.NumBlocks;
  }
  return S_OK;
}

struct CNodeDescriptor
{
  UInt32 fLink;
  UInt32 bLink;
  Byte Kind;
  Byte Height;
  UInt16 NumRecords;
  // UInt16 Reserved;
  void Parse(const Byte *p);
};

void CNodeDescriptor::Parse(const Byte *p)
{
  fLink = Get32(p);
  bLink = Get32(p + 4);
  Kind = p[8];
  Height = p[9];
  NumRecords = Get16(p + 10);
}

struct CHeaderRec
{
  // UInt16 TreeDepth;
  // UInt32 RootNode;
  // UInt32 LeafRecords;
  UInt32 FirstLeafNode;
  // UInt32 LastLeafNode;
  int NodeSizeLog;
  // UInt16 MaxKeyLength;
  UInt32 TotalNodes;
  // UInt32 FreeNodes;
  // UInt16 Reserved1;
  // UInt32 ClumpSize;
  // Byte BtreeType;
  // Byte KeyCompareType;
  // UInt32 Attributes;
  // UInt32 Reserved3[16];
  
  HRESULT Parse(const Byte *p);
};

HRESULT CHeaderRec::Parse(const Byte *p)
{
  // TreeDepth = Get16(p);
  // RootNode = Get32(p + 2);
  // LeafRecords = Get32(p + 6);
  FirstLeafNode = Get32(p + 0xA);
  // LastLeafNode = Get32(p + 0xE);
  UInt32 nodeSize = Get16(p + 0x12);

  int i;
  for (i = 9; ((UInt32)1 << i) != nodeSize; i++)
    if (i == 16)
      return S_FALSE;
  NodeSizeLog = i;

  // MaxKeyLength = Get16(p + 0x14);
  TotalNodes = Get32(p + 0x16);
  // FreeNodes = Get32(p + 0x1A);
  // Reserved1 = Get16(p + 0x1E);
  // ClumpSize = Get32(p + 0x20);
  // BtreeType = p[0x24];
  // KeyCompareType = p[0x25];
  // Attributes = Get32(p + 0x26);
  /*
  for (int i = 0; i < 16; i++)
    Reserved3[i] = Get32(p + 0x2A + i * 4);
  */
  return S_OK;
}


enum ENodeType
{
  NODE_TYPE_LEAF = 0xFF,
  NODE_TYPE_INDEX = 0,
  NODE_TYPE_HEADER = 1,
  NODE_TYPE_MODE = 2
};

HRESULT CDatabase::LoadExtentFile(IInStream *inStream)
{
  // FileExtents.Clear();
  // ResExtents.Clear();

  CByteBuffer extents;
  RINOK(ReadFile(Header.ExtentsFile, extents, inStream));

  const Byte *p = (const Byte *)extents;

  // CNodeDescriptor nodeDesc;
  // nodeDesc.Parse(p);
  CHeaderRec hr;
  RINOK(hr.Parse(p + 14));

  UInt32 node = hr.FirstLeafNode;
  if (node != 0)
    return S_FALSE;
  /*
  while (node != 0)
  {
    size_t nodeOffset = node * hr.NodeSize;
    if ((node + 1)* hr.NodeSize > CatalogBuf.GetCapacity())
      return S_FALSE;
    CNodeDescriptor desc;
    desc.Parse(p + nodeOffset);
    if (desc.Kind != NODE_TYPE_LEAF)
      return S_FALSE;
    UInt32 ptr = hr.NodeSize;
    for (int i = 0; i < desc.NumRecords; i++)
    {
      UInt32 offs = Get16(p + nodeOffset + hr.NodeSize - (i + 1) * 2);
      UInt32 offsNext = Get16(p + nodeOffset + hr.NodeSize - (i + 2) * 2);

      const Byte *r = p + nodeOffset + offs;
      int keyLength = Get16(r);
      Byte forkType = r[2];
      UInt32 id = Get16(r + 4);
      UInt32 startBlock = Get16(r + 4);
      CObjectVector<CIdExtents> *extents = (forkType == 0) ? &FileExtents : &ResExtents;
      if (extents->Size() == 0)
        extents->Add(CIdExtents());
      else
      {
        CIdExtents &e = extents->Back();
        if (e.ID != id)
        {
          if (e.ID > id)
            return S_FALSE;
          extents->Add(CIdExtents());
        }
      }
      CIdExtents &e = extents->Back();
      for (UInt32 k = offs + 10 + 2; k + 8 <= offsNext; k += 8)
      {
        CExtent ee;
        ee.Pos = Get32(p + nodeOffset + k);
        ee.NumBlocks  = Get32(p + nodeOffset + k * 4);
        e.Extents.Add(ee);
      }
    }
    node = desc.fLink;
  }
  */
  return S_OK;
}


HRESULT CDatabase::LoadCatalog(IInStream *inStream, CProgressVirt *progress)
{
  Items.Clear();
  IdToIndexMap.ClearAndFree();

  CByteBuffer catalogBuf;
  RINOK(ReadFile(Header.CatalogFile, catalogBuf, inStream));
  const Byte *p = (const Byte *)catalogBuf;

  // CNodeDescriptor nodeDesc;
  // nodeDesc.Parse(p);
  CHeaderRec hr;
  hr.Parse(p + 14);
  
  // CaseSensetive = (Header.IsHfsX() && hr.KeyCompareType == 0xBC);

  if ((catalogBuf.GetCapacity() >> hr.NodeSizeLog) < hr.TotalNodes)
    return S_FALSE;

  CByteBuffer usedBuf;
  usedBuf.SetCapacity(hr.TotalNodes);
  for (UInt32 i = 0; i < hr.TotalNodes; i++)
    usedBuf[i] = 0;

  UInt32 node = hr.FirstLeafNode;
  while (node != 0)
  {
    if (node >= hr.TotalNodes)
      return S_FALSE;
    if (usedBuf[node])
      return S_FALSE;
    usedBuf[node] = 1;
    size_t nodeOffset = (size_t)node << hr.NodeSizeLog;
    CNodeDescriptor desc;
    desc.Parse(p + nodeOffset);
    if (desc.Kind != NODE_TYPE_LEAF)
      return S_FALSE;
    for (int i = 0; i < desc.NumRecords; i++)
    {
      UInt32 nodeSize = (1 << hr.NodeSizeLog);
      UInt32 offs = Get16(p + nodeOffset + nodeSize - (i + 1) * 2);
      UInt32 offsNext = Get16(p + nodeOffset + nodeSize - (i + 2) * 2);
      UInt32 recSize = offsNext - offs;
      if (offsNext >= nodeSize || offsNext < offs || recSize < 6)
        return S_FALSE;

      CItem item;

      const Byte *r = p + nodeOffset + offs;
      UInt32 keyLength = Get16(r);
      item.ParentID = Get32(r + 2);
      UString name;
      if (keyLength < 6 || (keyLength & 1) != 0 || keyLength + 2 > recSize)
        return S_FALSE;
      r += 6;
      recSize -= 6;
      keyLength -= 6;
      
      int nameLength = Get16(r);
      if (nameLength * 2 != (int)keyLength)
        return S_FALSE;
      r += 2;
      recSize -= 2;
     
      wchar_t *pp = name.GetBuffer(nameLength + 1);
      
      int j;
      for (j = 0; j < nameLength; j++)
        pp[j] = ((wchar_t)r[j * 2] << 8) | r[j * 2 + 1];
      pp[j] = 0;
      name.ReleaseBuffer();
      r += j * 2;
      recSize -= j * 2;

      if (recSize < 2)
        return S_FALSE;
      item.Type = Get16(r);

      if (item.Type != RECORD_TYPE_FOLDER && item.Type != RECORD_TYPE_FILE)
        continue;
      if (recSize < 0x58)
        return S_FALSE;

      // item.Flags = Get16(r + 2);
      // item.Valence = Get32(r + 4);
      item.ID = Get32(r + 8);
      item.CTime = Get32(r + 0xC);
      item.MTime = Get32(r + 0x10);
      // item.AttrMTime = Get32(r + 0x14);
      item.ATime = Get32(r + 0x18);
      // item.BackupDate = Get32(r + 0x1C);
      
      /*
      item.OwnerID = Get32(r + 0x20);
      item.GroupID = Get32(r + 0x24);
      item.AdminFlags = r[0x28];
      item.OwnerFlags = r[0x29];
      item.FileMode = Get16(r + 0x2A);
      item.special.iNodeNum = Get16(r + 0x2C);
      */

      item.Name = name;

      if (item.IsDir())
      {
        CIdIndexPair pair;
        pair.ID = item.ID;
        pair.Index = Items.Size();
        IdToIndexMap.Add(pair);
      }
      else
      {
        CFork fd;
        recSize -= 0x58;
        r += 0x58;
        if (recSize < 0x50 * 2)
          return S_FALSE;
        fd.Parse(r);
        item.Size = fd.Size;
        item.NumBlocks = fd.NumBlocks;
        UInt32 curBlock = 0;
        for (int j = 0; j < 8; j++)
        {
          if (curBlock >= fd.NumBlocks)
            break;
          const CExtent &e = fd.Extents[j];
          item.Extents.Add(e);
          curBlock += e.NumBlocks;
        }
      }
      Items.Add(item);
      if (progress && Items.Size() % 100 == 0)
      {
        RINOK(progress->SetCompleted(Items.Size()));
      }
    }
    node = desc.fLink;
  }
  IdToIndexMap.Sort(CompareIdToIndex, NULL);
  return S_OK;
}

HRESULT CDatabase::Open(IInStream *inStream, CProgressVirt *progress)
{
  static const UInt32 kHeaderSize = 1024 + 512;
  Byte buf[kHeaderSize];
  RINOK(ReadStream_FALSE(inStream, buf, kHeaderSize));
  int i;
  for (i = 0; i < 1024; i++)
    if (buf[i] != 0)
      return S_FALSE;
  const Byte *p = buf + 1024;
  CVolHeader &h = Header;

  h.Header[0] = p[0];
  h.Header[1] = p[1];
  if (p[0] != 'H' || (p[1] != '+' && p[1] != 'X'))
    return S_FALSE;
  h.Version = Get16(p + 2);
  if (h.Version < 4 || h.Version > 5)
    return S_FALSE;

  // h.Attr = Get32(p + 4);
  // h.LastMountedVersion = Get32(p + 8);
  // h.JournalInfoBlock = Get32(p + 0xC);

  h.CTime = Get32(p + 0x10);
  h.MTime = Get32(p + 0x14);
  // h.BackupTime = Get32(p + 0x18);
  // h.CheckedTime = Get32(p + 0x1C);

  // h.NumFiles = Get32(p + 0x20);
  // h.NumFolders = Get32(p + 0x24);
  
  UInt32 numFiles = Get32(p + 0x20);
  UInt32 numFolders = Get32(p + 0x24);;
  if (progress)
  {
    RINOK(progress->SetTotal(numFolders + numFiles));
  }

  UInt32 blockSize = Get32(p + 0x28);

  for (i = 9; ((UInt32)1 << i) != blockSize; i++)
    if (i == 31)
      return S_FALSE;
  h.BlockSizeLog = i;

  h.NumBlocks = Get32(p + 0x2C);
  h.NumFreeBlocks = Get32(p + 0x30);

  /*
  h.WriteCount = Get32(p + 0x44);
  for (i = 0; i < 6; i++)
    h.FinderInfo[i] = Get32(p + 0x50 + i * 4);
  h.VolID = Get64(p + 0x68);
  */

  UInt64 endPos;
  RINOK(inStream->Seek(0, STREAM_SEEK_END, &endPos));
  if ((endPos >> h.BlockSizeLog) < h.NumBlocks)
    return S_FALSE;

  // h.AllocationFile.Parse(p + 0x70 + 0x50 * 0);
  h.ExtentsFile.Parse(   p + 0x70 + 0x50 * 1);
  h.CatalogFile.Parse(   p + 0x70 + 0x50 * 2);
  // h.AttributesFile.Parse(p + 0x70 + 0x50 * 3);
  // h.StartupFile.Parse(   p + 0x70 + 0x50 * 4);

  RINOK(LoadExtentFile(inStream));
  RINOK(LoadCatalog(inStream, progress));

  // if (Header.NumFiles + Header.NumFolders != (UInt32)Items.Size()) return S_OK;

  return S_OK;
}

}}
