// Archive/ChmIn.h

#ifndef __ARCHIVE_CHM_IN_H
#define __ARCHIVE_CHM_IN_H

#include "Common/Buffer.h"
#include "Common/MyString.h"

#include "../../IStream.h"
#include "../../Common/InBuffer.h"

#include "ChmHeader.h"

namespace NArchive {
namespace NChm {

struct CItem
{
  UInt64 Section;
  UInt64 Offset;
  UInt64 Size;
  AString Name;

  bool IsFormatRelatedItem() const
  {
    if (Name.Length() < 2)
      return false;
    return Name[0] == ':' && Name[1] == ':';
  }
  
  bool IsUserItem() const
  {
    if (Name.Length() < 2)
      return false;
    return Name[0] == '/';
  }
  
  bool IsDir() const
  {
    if (Name.Length() == 0)
      return false;
    return (Name[Name.Length() - 1] == '/');
  }
};

struct CDatabase
{
  UInt64 ContentOffset;
  CObjectVector<CItem> Items;
  AString NewFormatString;
  bool Help2Format;
  bool NewFormat;

  int FindItem(const AString &name) const
  {
    for (int i = 0; i < Items.Size(); i++)
      if (Items[i].Name == name)
        return i;
    return -1;
  }

  void Clear()
  {
    NewFormat = false;
    NewFormatString.Empty();
    Help2Format = false;
    Items.Clear();
  }
};

struct CResetTable
{
  UInt64 UncompressedSize;
  UInt64 CompressedSize;
  UInt64 BlockSize;
  CRecordVector<UInt64> ResetOffsets;
  bool GetCompressedSizeOfBlocks(UInt64 blockIndex, UInt32 numBlocks, UInt64 &size) const
  {
    if (blockIndex >= ResetOffsets.Size())
      return false;
    UInt64 startPos = ResetOffsets[(int)blockIndex];
    if (blockIndex + numBlocks >= ResetOffsets.Size())
      size = CompressedSize - startPos;
    else
      size = ResetOffsets[(int)(blockIndex + numBlocks)] - startPos;
    return true;
  }
  bool GetCompressedSizeOfBlock(UInt64 blockIndex, UInt64 &size) const
  {
    return GetCompressedSizeOfBlocks(blockIndex, 1, size);
  }
  UInt64 GetNumBlocks(UInt64 size) const
  {
    return (size + BlockSize - 1) / BlockSize;
  }
};

struct CLzxInfo
{
  UInt32 Version;
  UInt32 ResetInterval;
  UInt32 WindowSize;
  UInt32 CacheSize;
  CResetTable ResetTable;

  UInt32 GetNumDictBits() const
  {
    if (Version == 2 || Version == 3)
    {
      for (int i = 0; i <= 31; i++)
        if (((UInt32)1 << i) >= WindowSize)
          return 15 + i;
    }
    return 0;
  }

  UInt64 GetFolderSize() const { return ResetTable.BlockSize * ResetInterval; };
  UInt64 GetFolder(UInt64 offset) const { return offset / GetFolderSize(); };
  UInt64 GetFolderPos(UInt64 folderIndex) const { return folderIndex * GetFolderSize(); };
  UInt64 GetBlockIndexFromFolderIndex(UInt64 folderIndex) const { return folderIndex * ResetInterval; };
  bool GetOffsetOfFolder(UInt64 folderIndex, UInt64 &offset) const
  {
    UInt64 blockIndex = GetBlockIndexFromFolderIndex(folderIndex);
    if (blockIndex >= ResetTable.ResetOffsets.Size())
      return false;
    offset = ResetTable.ResetOffsets[(int)blockIndex];
    return true;
  }
  bool GetCompressedSizeOfFolder(UInt64 folderIndex, UInt64 &size) const
  {
    UInt64 blockIndex = GetBlockIndexFromFolderIndex(folderIndex);
    return ResetTable.GetCompressedSizeOfBlocks(blockIndex, ResetInterval, size);
  }
};

struct CMethodInfo
{
  GUID Guid;
  CByteBuffer ControlData;
  CLzxInfo LzxInfo;
  bool IsLzx() const;
  bool IsDes() const;
  AString GetGuidString() const;
  UString GetName() const;
};

struct CSectionInfo
{
  UInt64 Offset;
  UInt64 CompressedSize;
  UInt64 UncompressedSize;

  AString Name;
  CObjectVector<CMethodInfo> Methods;

  bool IsLzx() const;
  UString GetMethodName() const;
};

class CFilesDatabase: public CDatabase
{
public:
  bool LowLevel;
  CRecordVector<int> Indices;
  CObjectVector<CSectionInfo> Sections;

  UInt64 GetFileSize(int fileIndex) const { return Items[Indices[fileIndex]].Size; }
  UInt64 GetFileOffset(int fileIndex) const { return Items[Indices[fileIndex]].Offset; }

  UInt64 GetFolder(int fileIndex) const
  {
    const CItem &item = Items[Indices[fileIndex]];
    const CSectionInfo &section = Sections[(int)item.Section];
    if (section.IsLzx())
      return section.Methods[0].LzxInfo.GetFolder(item.Offset);
    return 0;
  }

  UInt64 GetLastFolder(int fileIndex) const
  {
    const CItem &item = Items[Indices[fileIndex]];
    const CSectionInfo &section = Sections[(int)item.Section];
    if (section.IsLzx())
      return section.Methods[0].LzxInfo.GetFolder(item.Offset + item.Size - 1);
    return 0;
  }

  void HighLevelClear()
  {
    LowLevel = true;
    Indices.Clear();
    Sections.Clear();
  }

  void Clear()
  {
    CDatabase::Clear();
    HighLevelClear();
  }
  void SetIndices();
  void Sort();
  bool Check();
};

class CProgressVirt
{
public:
  STDMETHOD(SetTotal)(const UInt64 *numFiles) PURE;
  STDMETHOD(SetCompleted)(const UInt64 *numFiles) PURE;
};

class CInArchive
{
  UInt64 _startPosition;
  ::CInBuffer _inBuffer;
  UInt64 _chunkSize;

  Byte ReadByte();
  void ReadBytes(Byte *data, UInt32 size);
  void Skip(size_t size);
  UInt16 ReadUInt16();
  UInt32 ReadUInt32();
  UInt64 ReadUInt64();
  UInt64 ReadEncInt();
  void ReadString(int size, AString &s);
  void ReadUString(int size, UString &s);
  void ReadGUID(GUID &g);

  HRESULT ReadChunk(IInStream *inStream, UInt64 pos, UInt64 size);

  HRESULT ReadDirEntry(CDatabase &database);
  HRESULT DecompressStream(IInStream *inStream, const CDatabase &database, const AString &name);

public:
  HRESULT OpenChm(IInStream *inStream, CDatabase &database);
  HRESULT OpenHelp2(IInStream *inStream, CDatabase &database);
  HRESULT OpenHighLevel(IInStream *inStream, CFilesDatabase &database);
  HRESULT Open2(IInStream *inStream, const UInt64 *searchHeaderSizeLimit, CFilesDatabase &database);
  HRESULT Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit, CFilesDatabase &database);
};
  
}}
  
#endif
