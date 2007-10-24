// Archive/ChmIn.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/MyCom.h"
#include "Common/UTFConvert.h"
#include "Common/IntToString.h"
#include "Windows/Defs.h"

#include "../../Common/LimitedStreams.h"
#include "ChmIn.h"

namespace NArchive{
namespace NChm{

// define CHM_LOW, if you want to see low level items
// #define CHM_LOW

static const GUID kChmLzxGuid = 
  { 0x7FC28940, 0x9D31, 0x11D0, 0x9B, 0x27, 0x00, 0xA0, 0xC9, 0x1E, 0x9C, 0x7C };
static const GUID kHelp2LzxGuid = 
  { 0x0A9007C6, 0x4076, 0x11D3, 0x87, 0x89, 0x00, 0x00, 0xF8, 0x10, 0x57, 0x54 };
static const GUID kDesGuid = 
  { 0x67F6E4A2, 0x60BF, 0x11D3, 0x85, 0x40, 0x00, 0xC0, 0x4F, 0x58, 0xC3, 0xCF };

static bool AreGuidsEqual(REFGUID g1, REFGUID g2)
{ 
  if (g1.Data1 != g2.Data1 || 
      g1.Data2 != g2.Data2 ||
      g1.Data3 != g2.Data3)
    return false;
  for (int i = 0; i < 8; i++)
    if (g1.Data4[i] != g2.Data4[i])
      return false;
  return true;
}

static char GetHex(Byte value)
{
  return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10)));
}

static void PrintByte(Byte b, AString &s)
{
  s += GetHex(b >> 4);
  s += GetHex(b & 0xF);
}

static void PrintUInt16(UInt16 v, AString &s)
{
  PrintByte((Byte)(v >> 8), s);
  PrintByte((Byte)v, s);
}

static void PrintUInt32(UInt32 v, AString &s)
{
  PrintUInt16((UInt16)(v >> 16), s);
  PrintUInt16((UInt16)v, s);
}

AString CMethodInfo::GetGuidString() const
{ 
  AString s;
  s += '{';
  PrintUInt32(Guid.Data1, s);
  s += '-';
  PrintUInt16(Guid.Data2, s);
  s += '-';
  PrintUInt16(Guid.Data3, s);
  s += '-';
  PrintByte(Guid.Data4[0], s);
  PrintByte(Guid.Data4[1], s);
  s += '-';
  for (int i = 2; i < 8; i++)
    PrintByte(Guid.Data4[i], s);
  s += '}';
  return s;
}

bool CMethodInfo::IsLzx() const
{
  if (AreGuidsEqual(Guid, kChmLzxGuid))
    return true;
  return AreGuidsEqual(Guid, kHelp2LzxGuid);
}

bool CMethodInfo::IsDes() const
{
  return AreGuidsEqual(Guid, kDesGuid);
}

UString CMethodInfo::GetName() const
{
  UString s;
  if (IsLzx())
  {
    s = L"LZX:";
    UInt32 numDictBits = LzxInfo.GetNumDictBits();
    wchar_t temp[32];
    ConvertUInt64ToString(numDictBits, temp);
    s += temp;
  }
  else
  {
    AString s2;
    if (IsDes())
      s2 = "DES";
    else
    {
      s2 = GetGuidString();
      if (ControlData.GetCapacity() > 0)
      {
        s2 += ":";
        for (size_t i = 0; i < ControlData.GetCapacity(); i++)
          PrintByte(ControlData[i], s2);
      }
    }
    ConvertUTF8ToUnicode(s2, s);
  }
  return s;
}

bool CSectionInfo::IsLzx() const
{
  if (Methods.Size() != 1)
    return false;
  return Methods[0].IsLzx();
}

UString CSectionInfo::GetMethodName() const
{
  UString s;
  if (!IsLzx())
  {
    UString temp;
    if (ConvertUTF8ToUnicode(Name, temp))
      s += temp;
    s += L": ";
  }
  for (int i = 0; i < Methods.Size(); i++)
  {
    if (i != 0)
      s += L" ";
    s += Methods[i].GetName();
  }
  return s;
}

Byte CInArchive::ReadByte()
{
  Byte b;
  if (!_inBuffer.ReadByte(b))
    throw 1;
  return b;
}

void CInArchive::Skeep(size_t size)
{
  while (size-- != 0)
    ReadByte();
}

void CInArchive::ReadBytes(Byte *data, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
    data[i] = ReadByte();
}

UInt16 CInArchive::ReadUInt16()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
    value |= ((UInt16)(ReadByte()) << (8 * i));
  return value;
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
    value |= ((UInt32)(ReadByte()) << (8 * i));
  return value;
}

UInt64 CInArchive::ReadUInt64()
{
  UInt64 value = 0;
  for (int i = 0; i < 8; i++)
    value |= ((UInt64)(ReadByte()) << (8 * i));
  return value;
}

UInt64 CInArchive::ReadEncInt()
{
  UInt64 val = 0;;
  for (int i = 0; i < 10; i++)
  {
    Byte b = ReadByte();
    val |= (b & 0x7F);
    if (b < 0x80)
      return val;
    val <<= 7;
  }
  throw 1;
}

void CInArchive::ReadGUID(GUID &g)
{
  g.Data1 = ReadUInt32();
  g.Data2 = ReadUInt16();
  g.Data3 = ReadUInt16();
  ReadBytes(g.Data4, 8);
}

void CInArchive::ReadString(int size, AString &s)
{
  s.Empty();
  while(size-- != 0)
  {
    char c = (char)ReadByte();
    if (c == 0)
    {
      Skeep(size);
      return;
    }
    s += c;
  }
}

void CInArchive::ReadUString(int size, UString &s)
{
  s.Empty();
  while(size-- != 0)
  {
    wchar_t c = ReadUInt16();
    if (c == 0)
    {
      Skeep(2 * size);
      return;
    }
    s += c;
  }
}

HRESULT CInArchive::ReadChunk(IInStream *inStream, UInt64 pos, UInt64 size)
{
  RINOK(inStream->Seek(pos, STREAM_SEEK_SET, NULL));
  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> limitedStream(streamSpec);
  streamSpec->SetStream(inStream);
  streamSpec->Init(size);
  _inBuffer.SetStream(limitedStream);
  _inBuffer.Init();
  return S_OK;
}

HRESULT CInArchive::ReadDirEntry(CDatabase &database)
{
  CItem item;
  UInt64 nameLength = ReadEncInt();
  if (nameLength == 0 || nameLength >= 0x10000000)
    return S_FALSE;
  ReadString((int)nameLength, item.Name);
  item.Section = ReadEncInt();
  item.Offset = ReadEncInt();
  item.Size = ReadEncInt();
  database.Items.Add(item);
  return S_OK;
}

HRESULT CInArchive::OpenChm(IInStream *inStream, CDatabase &database)
{
  UInt32 headerSize = ReadUInt32();
  if (headerSize != 0x60)
    return S_FALSE;
  UInt32 unknown1 = ReadUInt32();
  if (unknown1 != 0 && unknown1 != 1) // it's 0 in one .sll file
    return S_FALSE;
  /* UInt32 timeStamp = */ ReadUInt32();
      // Considered as a big-endian DWORD, it appears to contain seconds (MSB) and 
      // fractional seconds (second byte). 
      // The third and fourth bytes may contain even more fractional bits.  
      // The 4 least significant bits in the last byte are constant.
  /* UInt32 lang = */ ReadUInt32();
  GUID g;
  ReadGUID(g); // {7C01FD10-7BAA-11D0-9E0C-00A0-C922-E6EC}
  ReadGUID(g); // {7C01FD11-7BAA-11D0-9E0C-00A0-C922-E6EC}
  const int kNumSections = 2;
  UInt64 sectionOffsets[kNumSections];
  UInt64 sectionSizes[kNumSections];
  int i;
  for (i = 0; i < kNumSections; i++)
  {
    sectionOffsets[i] = ReadUInt64();
    sectionSizes[i] = ReadUInt64();
  }
  // if (chmVersion == 3)
    database.ContentOffset = ReadUInt64();
  /*
  else
    database.ContentOffset = _startPosition + 0x58
  */

  /*
  // Section 0
  ReadChunk(inStream, sectionOffsets[0], sectionSizes[0]);
  if (sectionSizes[0] != 0x18)
    return S_FALSE;
  ReadUInt32(); // unknown:  01FE
  ReadUInt32(); // unknown:  0
  UInt64 fileSize = ReadUInt64();
  ReadUInt32(); // unknown:  0
  ReadUInt32(); // unknown:  0
  */

  // Section 1: The Directory Listing
  ReadChunk(inStream, sectionOffsets[1], sectionSizes[1]);
  if (ReadUInt32() != NHeader::kItspSignature)
    return S_FALSE;
  if (ReadUInt32() != 1) // version
    return S_FALSE;
  /* UInt32 dirHeaderSize = */ ReadUInt32();
  ReadUInt32(); // 0x0A (unknown)
  UInt32 dirChunkSize = ReadUInt32(); // $1000
  if (dirChunkSize < 32)
    return S_FALSE;
  /* UInt32 density = */ ReadUInt32(); //  "Density" of quickref section, usually 2.
  /* UInt32 depth = */ ReadUInt32(); //  Depth of the index tree: 1 there is no index, 
                               // 2 if there is one level of PMGI chunks.

  /* UInt32 chunkNumber = */ ReadUInt32(); //  Chunk number of root index chunk, -1 if there is none
                                     // (though at least one file has 0 despite there being no 
                                     // index chunk, probably a bug.) 
  /* UInt32 firstPmglChunkNumber = */ ReadUInt32(); // Chunk number of first PMGL (listing) chunk
  /* UInt32 lastPmglChunkNumber = */ ReadUInt32();  // Chunk number of last PMGL (listing) chunk
  ReadUInt32(); // -1 (unknown)
  UInt32 numDirChunks = ReadUInt32(); // Number of directory chunks (total)
  /* UInt32 windowsLangId = */ ReadUInt32();
  ReadGUID(g);  // {5D02926A-212E-11D0-9DF9-00A0C922E6EC}
  ReadUInt32(); // 0x54 (This is the length again)
  ReadUInt32(); // -1 (unknown)
  ReadUInt32(); // -1 (unknown)
  ReadUInt32(); // -1 (unknown)

  for (UInt32 ci = 0; ci < numDirChunks; ci++)
  {
    UInt64 chunkPos = _inBuffer.GetProcessedSize();
    if (ReadUInt32() == NHeader::kPmglSignature)
    {
      // The quickref area is written backwards from the end of the chunk. 
      // One quickref entry exists for every n entries in the file, where n 
      // is calculated as 1 + (1 << quickref density). So for density = 2, n = 5. 

      UInt32 quickrefLength = ReadUInt32(); // Length of free space and/or quickref area at end of directory chunk 
      if (quickrefLength > dirChunkSize || quickrefLength < 2)
        return S_FALSE;
      ReadUInt32(); // Always 0
      ReadUInt32(); // Chunk number of previous listing chunk when reading
                    // directory in sequence (-1 if this is the first listing chunk)
      ReadUInt32(); // Chunk number of next  listing chunk when reading
                    // directory in sequence (-1 if this is the last listing chunk)
      int numItems = 0;
      for (;;)
      {
        UInt64 offset = _inBuffer.GetProcessedSize() - chunkPos;
        UInt32 offsetLimit = dirChunkSize - quickrefLength;
        if (offset > offsetLimit)
          return S_FALSE;
        if (offset == offsetLimit)
          break;
        RINOK(ReadDirEntry(database));
        numItems++;
      }
      Skeep(quickrefLength - 2);
      if (ReadUInt16() != numItems)
        return S_FALSE;
    }
    else
      Skeep(dirChunkSize - 4);
  }
  return S_OK;
}

HRESULT CInArchive::OpenHelp2(IInStream *inStream, CDatabase &database)
{
  if (ReadUInt32() != 1) // version
    return S_FALSE;
  if (ReadUInt32() != 0x28) // Location of header section table
    return S_FALSE;
  UInt32 numHeaderSections = ReadUInt32();
  const int kNumHeaderSectionsMax = 5;
  if (numHeaderSections != kNumHeaderSectionsMax)
    return S_FALSE;
  ReadUInt32(); // Length of post-header table
  GUID g; 
  ReadGUID(g);  // {0A9007C1-4076-11D3-8789-0000F8105754}

  // header section table 
  UInt64 sectionOffsets[kNumHeaderSectionsMax];
  UInt64 sectionSizes[kNumHeaderSectionsMax];
  UInt32 i;
  for (i = 0; i < numHeaderSections; i++)
  {
    sectionOffsets[i] = ReadUInt64();
    sectionSizes[i] = ReadUInt64();
  }
  
  // Post-Header
  ReadUInt32(); // 2
  ReadUInt32(); // 0x98: offset to CAOL from beginning of post-header)
  // ----- Directory information
  ReadUInt64(); // Chunk number of top-level AOLI chunk in directory, or -1
  ReadUInt64(); // Chunk number of first AOLL chunk in directory
  ReadUInt64(); // Chunk number of last AOLL chunk in directory
  ReadUInt64(); // 0 (unknown)
  ReadUInt32(); // $2000 (Directory chunk size of directory)
  ReadUInt32(); // Quickref density for main directory, usually 2
  ReadUInt32(); // 0 (unknown)
  ReadUInt32(); // Depth of main directory index tree
                // 1 there is no index, 2 if there is one level of AOLI chunks.
  ReadUInt64(); // 0 (unknown)
  UInt64 numDirEntries = ReadUInt64(); // Number of directory entries
  // ----- Directory Index Information
  ReadUInt64(); // -1 (unknown, probably chunk number of top-level AOLI in directory index)
  ReadUInt64(); // Chunk number of first AOLL chunk in directory index
  ReadUInt64(); // Chunk number of last AOLL chunk in directory index
  ReadUInt64(); // 0 (unknown)
  ReadUInt32(); // $200 (Directory chunk size of directory index)
  ReadUInt32(); // Quickref density for directory index, usually 2
  ReadUInt32(); // 0 (unknown)
  ReadUInt32(); // Depth of directory index index tree.
  ReadUInt64(); // Possibly flags -- sometimes 1, sometimes 0.
  ReadUInt64(); // Number of directory index entries (same as number of AOLL
               // chunks in main directory)
  
  // (The obvious guess for the following two fields, which recur in a number 
  // of places, is they are maximum sizes for the directory and directory index. 
  // However, I have seen no direct evidence that this is the case.) 

  ReadUInt32(); // $100000 (Same as field following chunk size in directory)
  ReadUInt32(); // $20000 (Same as field following chunk size in directory index)

  ReadUInt64(); // 0 (unknown)
  if (ReadUInt32() != NHeader::kCaolSignature)
    return S_FALSE;
  if (ReadUInt32() != 2) // (Most likely a version number)
    return S_FALSE;
  UInt32 caolLength = ReadUInt32(); // $50 (Length of the CAOL section, which includes the ITSF section)
  if (caolLength >= 0x2C)
  {
    /* UInt32 c7 = */ ReadUInt16(); // Unknown.  Remains the same when identical files are built.
              // Does not appear to be a checksum.  Many files have
              // 'HH' (HTML Help?) here, indicating this may be a compiler ID
              //  field.  But at least one ITOL/ITLS compiler does not set this
              // field to a constant value.
    ReadUInt16(); // 0 (Unknown.  Possibly part of 00A4 field)
    ReadUInt32(); // Unknown.  Two values have been seen -- $43ED, and 0.
    ReadUInt32(); // $2000 (Directory chunk size of directory)
    ReadUInt32(); // $200 (Directory chunk size of directory index)
    ReadUInt32(); // $100000 (Same as field following chunk size in directory)
    ReadUInt32(); // $20000 (Same as field following chunk size in directory index)
    ReadUInt32(); // 0 (unknown)
    ReadUInt32(); // 0 (Unknown)
    if (caolLength == 0x2C)
    {
      database.ContentOffset = 0;
      database.NewFormat = true;
    }
    else if (caolLength == 0x50)
    {
      ReadUInt32(); // 0 (Unknown)
      if (ReadUInt32() != NHeader::kItsfSignature)
        return S_FALSE;
      if (ReadUInt32() != 4) // $4 (Version number -- CHM uses 3)
        return S_FALSE;
      if (ReadUInt32() != 0x20) // $20 (length of ITSF)
        return S_FALSE;
      UInt32 unknown = ReadUInt32();
      if (unknown != 0 && unknown != 1) // = 0 for some HxW files, 1 in other cases;
        return S_FALSE;
      database.ContentOffset = _startPosition + ReadUInt64();
      /* UInt32 timeStamp = */ ReadUInt32(); 
          // A timestamp of some sort.  
          // Considered as a big-endian DWORD, it appears to contain
          // seconds (MSB) and fractional seconds (second byte).
          // The third and fourth bytes may contain even more fractional
          // bits.  The 4 least significant bits in the last byte are constant.
      /* UInt32 lang = */ ReadUInt32(); // BE?
    }
    else
      return S_FALSE;
  }

  /*
  // Section 0
  ReadChunk(inStream, _startPosition + sectionOffsets[0], sectionSizes[0]);
  if (sectionSizes[0] != 0x18)
    return S_FALSE;
  ReadUInt32(); // unknown:  01FE
  ReadUInt32(); // unknown:  0
  UInt64 fileSize = ReadUInt64();
  ReadUInt32(); // unknown:  0
  ReadUInt32(); // unknown:  0
  */

  // Section 1: The Directory Listing
  ReadChunk(inStream, _startPosition + sectionOffsets[1], sectionSizes[1]);
  if (ReadUInt32() != NHeader::kIfcmSignature)
    return S_FALSE;
  if (ReadUInt32() != 1) // (probably a version number)
    return S_FALSE;
  UInt32 dirChunkSize = ReadUInt32(); // $2000
  if (dirChunkSize < 64)
    return S_FALSE;
  ReadUInt32(); // $100000  (unknown)
  ReadUInt32(); // -1 (unknown)
  ReadUInt32(); // -1 (unknown)
  UInt32 numDirChunks = ReadUInt32();
  ReadUInt32(); // 0 (unknown, probably high word of above)

  for (UInt32 ci = 0; ci < numDirChunks; ci++)
  {
    UInt64 chunkPos = _inBuffer.GetProcessedSize();
    if (ReadUInt32() == NHeader::kAollSignature)
    {
      UInt32 quickrefLength = ReadUInt32(); // Length of quickref area at end of directory chunk
      if (quickrefLength > dirChunkSize || quickrefLength < 2)
        return S_FALSE;
      ReadUInt64(); // Directory chunk number 
            // This must match physical position in file, that is
            // the chunk size times the chunk number must be the
            // offset from the end of the directory header.
      ReadUInt64(); // Chunk number of previous listing chunk when reading
                    // directory in sequence (-1 if first listing chunk)
      ReadUInt64(); // Chunk number of next listing chunk when reading
                    // directory in sequence (-1 if last listing chunk)
      ReadUInt64(); // Number of first listing entry in this chunk
      ReadUInt32(); // 1 (unknown -- other values have also been seen here)
      ReadUInt32(); // 0 (unknown)
      
      int numItems = 0;
      for (;;)
      {
        UInt64 offset = _inBuffer.GetProcessedSize() - chunkPos;
        UInt32 offsetLimit = dirChunkSize - quickrefLength;
        if (offset > offsetLimit)
          return S_FALSE;
        if (offset == offsetLimit)
          break;
        if (database.NewFormat)
        {
          UInt16 nameLength = ReadUInt16();
          if (nameLength == 0)
            return S_FALSE;
          UString name;
          ReadUString((int)nameLength, name);
          AString s;
          ConvertUnicodeToUTF8(name, s);
          Byte b = ReadByte();
          s += ' ';
          PrintByte(b, s);
          s += ' ';
          UInt64 len = ReadEncInt();
          // then number of items ?
          // then length ?
          // then some data (binary encoding?)
          while (len-- != 0)
          {
            b = ReadByte();
            PrintByte(b, s);
          }
          database.NewFormatString += s;
          database.NewFormatString += "\r\n";
        }
        else
        {
          RINOK(ReadDirEntry(database));
        }
        numItems++;
      }
      Skeep(quickrefLength - 2);
      if (ReadUInt16() != numItems)
        return S_FALSE;
      if (numItems > numDirEntries)
        return S_FALSE;
      numDirEntries -= numItems;
    }
    else
      Skeep(dirChunkSize - 4);
  }
  return numDirEntries == 0 ? S_OK : S_FALSE;
}

HRESULT CInArchive::DecompressStream(IInStream *inStream, const CDatabase &database, const AString &name)
{
  int index = database.FindItem(name);
  if (index < 0)
    return S_FALSE;
  const CItem &item = database.Items[index];
  _chunkSize = item.Size;
  return ReadChunk(inStream, database.ContentOffset + item.Offset, item.Size);
}


#define DATA_SPACE "::DataSpace/"
static const char *kNameList = DATA_SPACE "NameList";
static const char *kStorage = DATA_SPACE "Storage/";
static const char *kContent = "Content";
static const char *kControlData = "ControlData";
static const char *kSpanInfo = "SpanInfo";
static const char *kTransform = "Transform/";
static const char *kResetTable = "/InstanceData/ResetTable";
static const char *kTransformList = "List";

static AString GetSectionPrefix(const AString &name)
{
  return AString(kStorage) + name + AString("/");
}

#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

static int CompareFiles(const int *p1, const int *p2, void *param)
{
  const CObjectVector<CItem> &items = *(const CObjectVector<CItem> *)param;
  const CItem &item1 = items[*p1];
  const CItem &item2 = items[*p2];
  bool isDir1 = item1.IsDirectory();
  bool isDir2 = item2.IsDirectory();
  if (isDir1 && !isDir2)
    return -1;
  if (isDir2)
  {
    if (isDir1)
      return MyCompare(*p1, *p2);
    return 1;
  }
  RINOZ(MyCompare(item1.Section, item2.Section));
  RINOZ(MyCompare(item1.Offset, item2.Offset));
  RINOZ(MyCompare(item1.Size, item2.Size));
  return MyCompare(*p1, *p2);
}

void CFilesDatabase::SetIndices()
{
  for (int i = 0; i < Items.Size(); i++)
  {
    const CItem &item = Items[i];
    if (item.IsUserItem() && item.Name.Length() != 1)
      Indices.Add(i);
  }
}

void CFilesDatabase::Sort()
{
  Indices.Sort(CompareFiles, (void *)&Items);
}

bool CFilesDatabase::Check()
{
  UInt64 maxPos = 0;
  UInt64 prevSection = 0;
  for(int i = 0; i < Indices.Size(); i++)
  {
    const CItem &item = Items[Indices[i]];
    if (item.Section == 0 || item.IsDirectory())
      continue;
    if (item.Section != prevSection)
    {
      prevSection = item.Section;
      maxPos = 0;
      continue;
    }
    if (item.Offset < maxPos)
      return false;
    maxPos = item.Offset + item.Size;
    if (maxPos < item.Offset)
      return false;
  }
  return true;
}

HRESULT CInArchive::OpenHighLevel(IInStream *inStream, CFilesDatabase &database)
{
  {
    // The NameList file
    RINOK(DecompressStream(inStream, database, kNameList));
    /* UInt16 length = */ ReadUInt16();
    UInt16 numSections = ReadUInt16();
    for (int i = 0; i < numSections; i++)
    {
      CSectionInfo section;
      UInt16 nameLength  = ReadUInt16();
      UString name;
      ReadUString(nameLength, name);
      if (ReadUInt16() != 0)
        return S_FALSE;
      if (!ConvertUnicodeToUTF8(name, section.Name))
        return S_FALSE;
      database.Sections.Add(section);
    }
  }

  int i;
  for (i = 1; i < database.Sections.Size(); i++)
  {
    CSectionInfo &section = database.Sections[i];
    AString sectionPrefix = GetSectionPrefix(section.Name);
    {
      // Content
      int index = database.FindItem(sectionPrefix + kContent);
      if (index < 0)
        return S_FALSE;
      const CItem &item = database.Items[index];
      section.Offset = item.Offset;
      section.CompressedSize = item.Size;
    }
    AString transformPrefix = sectionPrefix + kTransform;
    if (database.Help2Format)
    {
      // Transform List 
      RINOK(DecompressStream(inStream, database, transformPrefix + kTransformList));
      if ((_chunkSize & 0xF) != 0)
        return S_FALSE;
      int numGuids = (int)(_chunkSize / 0x10);
      if (numGuids < 1)
        return S_FALSE;
      for (int i = 0; i < numGuids; i++)
      {
        CMethodInfo method;
        ReadGUID(method.Guid);
        section.Methods.Add(method);
      }
    }
    else
    {
      CMethodInfo method;
      method.Guid = kChmLzxGuid;
      section.Methods.Add(method);
    }

    {
      // Control Data
      RINOK(DecompressStream(inStream, database, sectionPrefix + kControlData));
      for (int mi = 0; mi < section.Methods.Size(); mi++)
      {
        CMethodInfo &method = section.Methods[mi];
        UInt32 numDWORDS = ReadUInt32();
        if (method.IsLzx())
        {
          if (numDWORDS < 5)
            return S_FALSE;
          if (ReadUInt32() != NHeader::kLzxcSignature)
            return S_FALSE;
          CLzxInfo &li = method.LzxInfo;
          li.Version = ReadUInt32();
          if (li.Version != 2 && li.Version != 3)
            return S_FALSE;
          li.ResetInterval = ReadUInt32();
          li.WindowSize = ReadUInt32();
          li.CacheSize = ReadUInt32();
          if (
              li.ResetInterval != 1 && 
              li.ResetInterval != 2 && 
              li.ResetInterval != 4 && 
              li.ResetInterval != 8 && 
              li.ResetInterval != 16 && 
              li.ResetInterval != 32 && 
              li.ResetInterval != 64)
            return S_FALSE;
          if (
              li.WindowSize != 1 && 
              li.WindowSize != 2 && 
              li.WindowSize != 4 && 
              li.WindowSize != 8 && 
              li.WindowSize != 16 && 
              li.WindowSize != 32 && 
              li.WindowSize != 64)
            return S_FALSE;
          numDWORDS -= 5;
          while (numDWORDS-- != 0)
            ReadUInt32();
        }
        else
        {
          UInt32 numBytes = numDWORDS * 4;
          method.ControlData.SetCapacity(numBytes);
          ReadBytes(method.ControlData, numBytes);
        }
      }
    }

    {
      // SpanInfo
      RINOK(DecompressStream(inStream, database, sectionPrefix + kSpanInfo));
      section.UncompressedSize = ReadUInt64();
    }

    // read ResetTable for LZX
    for (int mi = 0; mi < section.Methods.Size(); mi++)
    {
      CMethodInfo &method = section.Methods[mi];
      if (method.IsLzx())
      {
        // ResetTable;
        RINOK(DecompressStream(inStream, database, transformPrefix + 
            method.GetGuidString() + kResetTable));
        CResetTable &rt = method.LzxInfo.ResetTable;
        if (_chunkSize < 4)
        {
          if (_chunkSize != 0)
            return S_FALSE;
          // ResetTable is empty in .chw files
          if (section.UncompressedSize != 0)
            return S_FALSE;
          rt.UncompressedSize = 0;
          rt.CompressedSize = 0;
          rt.BlockSize = 0;
        }
        else
        {
          UInt32 ver = ReadUInt32(); // 2  unknown (possibly a version number)
          if (ver != 2 && ver != 3)
            return S_FALSE;
          UInt32 numEntries = ReadUInt32();
          if (ReadUInt32() != 8) // Size of table entry (bytes)
            return S_FALSE;
          if (ReadUInt32() != 0x28) // Length of table header
            return S_FALSE;
          rt.UncompressedSize = ReadUInt64();
          rt.CompressedSize = ReadUInt64();
          rt.BlockSize = ReadUInt64(); //  0x8000 block size for locations below
          if (rt.BlockSize != 0x8000)
            return S_FALSE;
          rt.ResetOffsets.Reserve(numEntries);
          for (UInt32 i = 0; i < numEntries; i++)
            rt.ResetOffsets.Add(ReadUInt64());
        }
      }
    }
  }

  database.SetIndices();
  database.Sort();
  return database.Check() ? S_OK : S_FALSE;
}

HRESULT CInArchive::Open2(IInStream *inStream, 
    const UInt64 *searchHeaderSizeLimit,
    CFilesDatabase &database)
{
  database.Clear();

  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &_startPosition));

  database.Help2Format = false;
  const UInt32 chmVersion = 3;
  {
    if (!_inBuffer.Create(1 << 14))
      return E_OUTOFMEMORY;
    _inBuffer.SetStream(inStream);
    _inBuffer.Init();
    UInt64 value = 0;
    const int kSignatureSize = 8;
    UInt64 hxsSignature = NHeader::GetHxsSignature();
    UInt64 chmSignature = ((UInt64)chmVersion << 32)| NHeader::kItsfSignature;
    for (;;)
    {
      Byte b;
      if (!_inBuffer.ReadByte(b))
        return S_FALSE;
      value >>= 8;
      value |= ((UInt64)b) << ((kSignatureSize - 1) * 8);
      if (_inBuffer.GetProcessedSize() >= kSignatureSize)
      {
        if (value == chmSignature)
          break;
        if (value == hxsSignature)
        {
          database.Help2Format = true;
          break;
        }
        if (searchHeaderSizeLimit != NULL)
          if (_inBuffer.GetProcessedSize() > (*searchHeaderSizeLimit))
            return S_FALSE;
      }
    }
    _startPosition += _inBuffer.GetProcessedSize() - kSignatureSize;
  }

  if (database.Help2Format)
  {
    RINOK(OpenHelp2(inStream, database));
    if (database.NewFormat)
      return S_OK;
  }
  else
  {
    RINOK(OpenChm(inStream, database));
  }

  #ifndef CHM_LOW
  try
  {
    HRESULT res = OpenHighLevel(inStream, database);
    if (res == S_FALSE)
    {
      database.HighLevelClear();
      return S_OK;
    }
    RINOK(res);
    database.LowLevel = false;
  }
  catch(...)
  {
    return S_OK;
  }
  #endif
  return S_OK;
}

HRESULT CInArchive::Open(IInStream *inStream, 
    const UInt64 *searchHeaderSizeLimit,
    CFilesDatabase &database)
{
  try
  {
    HRESULT res = Open2(inStream, searchHeaderSizeLimit, database);
    _inBuffer.ReleaseStream();
    return res;
  }
  catch(...)
  {
    _inBuffer.ReleaseStream();
    throw;
  }
}

}}
