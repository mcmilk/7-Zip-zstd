// TarIn.cpp

#include "StdAfx.h"

#include "../../../../C/CpuArch.h"

#include "Common/StringToInt.h"

#include "../../Common/StreamUtils.h"

#include "TarIn.h"

namespace NArchive {
namespace NTar {
 
static void MyStrNCpy(char *dest, const char *src, int size)
{
  for (int i = 0; i < size; i++)
  {
    char c = src[i];
    dest[i] = c;
    if (c == 0)
      break;
  }
}

static bool OctalToNumber(const char *srcString, int size, UInt64 &res)
{
  char sz[32];
  MyStrNCpy(sz, srcString, size);
  sz[size] = 0;
  const char *end;
  int i;
  for (i = 0; sz[i] == ' '; i++);
  res = ConvertOctStringToUInt64(sz + i, &end);
  return (*end == ' ' || *end == 0);
}

static bool OctalToNumber32(const char *srcString, int size, UInt32 &res)
{
  UInt64 res64;
  if (!OctalToNumber(srcString, size, res64))
    return false;
  res = (UInt32)res64;
  return (res64 <= 0xFFFFFFFF);
}

#define RIF(x) { if (!(x)) return S_FALSE; }

static bool IsRecordLast(const char *buf)
{
  for (int i = 0; i < NFileHeader::kRecordSize; i++)
    if (buf[i] != 0)
      return false;
  return true;
}

static void ReadString(const char *s, int size, AString &result)
{
  char temp[NFileHeader::kRecordSize + 1];
  MyStrNCpy(temp, s, size);
  temp[size] = '\0';
  result = temp;
}

static bool ParseInt64(const char *p, Int64 &val)
{
  UInt32 h = GetBe32(p);
  val = GetBe64(p + 4);
  if (h == (UInt32)1 << 31)
    return ((val >> 63) & 1) == 0;
  if (h == (UInt32)(Int32)-1)
    return ((val >> 63) & 1) != 0;
  UInt64 uv;
  bool res = OctalToNumber(p, 12, uv);
  val = uv;
  return res;
}

static bool ParseSize(const char *p, UInt64 &val)
{
  if (GetBe32(p) == (UInt32)1 << 31)
  {
    // GNU extension
    val = GetBe64(p + 4);
    return ((val >> 63) & 1) == 0;
  }
  return OctalToNumber(p, 12, val);
}

static HRESULT GetNextItemReal(ISequentialInStream *stream, bool &filled, CItemEx &item, AString &error)
{
  char buf[NFileHeader::kRecordSize];
  char *p = buf;

  error.Empty();
  filled = false;

  bool thereAreEmptyRecords = false;
  for (;;)
  {
    size_t processedSize = NFileHeader::kRecordSize;
    RINOK(ReadStream(stream, buf, &processedSize));
    if (processedSize == 0)
    {
      if (!thereAreEmptyRecords )
        error = "There are no trailing zero-filled records";
      return S_OK;
    }
    if (processedSize != NFileHeader::kRecordSize)
    {
      error = "There is no correct record at the end of archive";
      return S_OK;
    }
    item.HeaderSize += NFileHeader::kRecordSize;
    if (!IsRecordLast(buf))
      break;
    thereAreEmptyRecords = true;
  }
  if (thereAreEmptyRecords)
  {
    error = "There are data after end of archive";
    return S_OK;
  }
  
  ReadString(p, NFileHeader::kNameSize, item.Name); p += NFileHeader::kNameSize;

  RIF(OctalToNumber32(p, 8, item.Mode)); p += 8;

  if (!OctalToNumber32(p, 8, item.UID)) item.UID = 0; p += 8;
  if (!OctalToNumber32(p, 8, item.GID)) item.GID = 0; p += 8;

  RIF(ParseSize(p, item.PackSize));
  item.Size = item.PackSize;
  p += 12;
  RIF(ParseInt64(p, item.MTime)); p += 12;
  
  UInt32 checkSum;
  RIF(OctalToNumber32(p, 8, checkSum));
  memcpy(p, NFileHeader::kCheckSumBlanks, 8); p += 8;

  item.LinkFlag = *p++;

  ReadString(p, NFileHeader::kNameSize, item.LinkName); p += NFileHeader::kNameSize;

  memcpy(item.Magic, p, 8); p += 8;

  ReadString(p, NFileHeader::kUserNameSize, item.User); p += NFileHeader::kUserNameSize;
  ReadString(p, NFileHeader::kGroupNameSize, item.Group); p += NFileHeader::kGroupNameSize;

  item.DeviceMajorDefined = (p[0] != 0); RIF(OctalToNumber32(p, 8, item.DeviceMajor)); p += 8;
  item.DeviceMinorDefined = (p[0] != 0); RIF(OctalToNumber32(p, 8, item.DeviceMinor)); p += 8;

  AString prefix;
  ReadString(p, NFileHeader::kPrefixSize, prefix);
  p += NFileHeader::kPrefixSize;
  if (!prefix.IsEmpty() && item.IsMagic() &&
      (item.LinkFlag != 'L' /* || prefix != "00000000000" */ ))
    item.Name = prefix + AString('/') + item.Name;

  if (item.LinkFlag == NFileHeader::NLinkFlag::kLink)
  {
    item.PackSize = 0;
    item.Size = 0;
  }
  else if (item.LinkFlag == NFileHeader::NLinkFlag::kSparse)
  {
    if (buf[482] != 0)
      return S_FALSE;
    RIF(ParseSize(buf + 483, item.Size));
    p = buf + 386;
    UInt64 min = 0;
    for (int i = 0; i < 4; i++, p += 24)
    {
      if (GetBe32(p) == 0)
        break;
      CSparseBlock sb;
      RIF(ParseSize(p, sb.Offset));
      RIF(ParseSize(p + 12, sb.Size));
      item.SparseBlocks.Add(sb);
      if (sb.Offset < min || sb.Offset > item.Size)
        return S_FALSE;
      if ((sb.Offset & 0x1FF) != 0 || (sb.Size & 0x1FF) != 0)
        return S_FALSE;
      min = sb.Offset + sb.Size;
      if (min < sb.Offset)
        return S_FALSE;
    }
    if (min > item.Size)
      return S_FALSE;
  }
 
  UInt32 checkSumReal = 0;
  for (int i = 0; i < NFileHeader::kRecordSize; i++)
    checkSumReal += (Byte)buf[i];
  
  if (checkSumReal != checkSum)
    return S_FALSE;

  filled = true;
  return S_OK;
}

HRESULT ReadItem(ISequentialInStream *stream, bool &filled, CItemEx &item, AString &error)
{
  item.HeaderSize = 0;
  bool flagL = false;
  bool flagK = false;
  AString nameL;
  AString nameK;
  for (;;)
  {
    RINOK(GetNextItemReal(stream, filled, item, error));
    if (!filled)
      return S_OK;
    if (item.LinkFlag == 'L' || // NEXT file has a long name
        item.LinkFlag == 'K') // NEXT file has a long linkname
    {
      AString *name;
      if (item.LinkFlag == 'L')
        { if (flagL) return S_FALSE; flagL = true; name = &nameL; }
      else
        { if (flagK) return S_FALSE; flagK = true; name = &nameK; }

      if (item.Name.Compare(NFileHeader::kLongLink) != 0 &&
          item.Name.Compare(NFileHeader::kLongLink2) != 0)
        return S_FALSE;
      if (item.PackSize > (1 << 14))
        return S_FALSE;
      int packSize = (int)item.GetPackSizeAligned();
      char *buf = name->GetBuffer(packSize);
      RINOK(ReadStream_FALSE(stream, buf, packSize));
      item.HeaderSize += packSize;
      buf[(size_t)item.PackSize] = '\0';
      name->ReleaseBuffer();
      continue;
    }
    switch (item.LinkFlag)
    {
      case 'g':
      case 'x':
      case 'X':
      {
        // pax Extended Header
        break;
      }
      case NFileHeader::NLinkFlag::kDumpDir:
      {
        break;
        // GNU Extensions to the Archive Format
      }
      case NFileHeader::NLinkFlag::kSparse:
      {
        break;
        // GNU Extensions to the Archive Format
      }
      default:
        if (item.LinkFlag > '7' || (item.LinkFlag < '0' && item.LinkFlag != 0))
          return S_FALSE;
    }
    if (flagL) item.Name = nameL;
    if (flagK) item.LinkName = nameK;
    return S_OK;
  }
}

}}
