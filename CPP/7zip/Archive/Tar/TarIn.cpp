// TarIn.cpp

#include "StdAfx.h"

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

static HRESULT GetNextItemReal(ISequentialInStream *stream, bool &filled, CItemEx &item, size_t &processedSize)
{
  item.LongLinkSize = 0;
  char buf[NFileHeader::kRecordSize];
  char *p = buf;

  filled = false;

  processedSize = NFileHeader::kRecordSize;
  RINOK(ReadStream(stream, buf, &processedSize));
  if (processedSize == 0 || (processedSize == NFileHeader::kRecordSize && IsRecordLast(buf)))
    return S_OK;
  if (processedSize < NFileHeader::kRecordSize)
    return S_FALSE;
  
  ReadString(p, NFileHeader::kNameSize, item.Name); p += NFileHeader::kNameSize;

  RIF(OctalToNumber32(p, 8, item.Mode)); p += 8;

  if (!OctalToNumber32(p, 8, item.UID)) item.UID = 0; p += 8;
  if (!OctalToNumber32(p, 8, item.GID)) item.GID = 0; p += 8;

  RIF(OctalToNumber(p, 12, item.Size)); p += 12;
  RIF(OctalToNumber32(p, 12, item.MTime)); p += 12;
  
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
    item.Size = 0;
 
  UInt32 checkSumReal = 0;
  for (int i = 0; i < NFileHeader::kRecordSize; i++)
    checkSumReal += (Byte)buf[i];
  
  if (checkSumReal != checkSum)
    return S_FALSE;

  filled = true;
  return S_OK;
}

HRESULT ReadItem(ISequentialInStream *stream, bool &filled, CItemEx &item)
{
  size_t processedSize;
  RINOK(GetNextItemReal(stream, filled, item, processedSize));
  if (!filled)
    return S_OK;
  // GNUtar extension
  if (item.LinkFlag == 'L' || // NEXT file has a long name
      item.LinkFlag == 'K') // NEXT file has a long linkname
  {
    if (item.Name.Compare(NFileHeader::kLongLink) != 0)
      if (item.Name.Compare(NFileHeader::kLongLink2) != 0)
        return S_FALSE;

    AString fullName;
    if (item.Size > (1 << 15))
      return S_FALSE;
    int packSize = (int)item.GetPackSize();
    char *buffer = fullName.GetBuffer(packSize + 1);

    RINOK(ReadStream_FALSE(stream, buffer, packSize));
    processedSize += packSize;
    buffer[item.Size] = '\0';
    fullName.ReleaseBuffer();

    UInt64 headerPosition = item.HeaderPosition;
    if (item.LinkFlag == 'L')
    {
      size_t processedSize2;
      RINOK(GetNextItemReal(stream, filled, item, processedSize2));
      item.LongLinkSize = (unsigned)processedSize;
    }
    else
    {
      item.LongLinkSize = (unsigned)processedSize - NFileHeader::kRecordSize;
      item.Size = 0;
    }
    item.Name = fullName;
    item.HeaderPosition = headerPosition;
  }
  else if (item.LinkFlag == 'g' || item.LinkFlag == 'x' || item.LinkFlag == 'X')
  {
    // pax Extended Header
    return S_OK;
  }
  else if (item.LinkFlag == NFileHeader::NLinkFlag::kDumpDir)
  {
    // GNU Extensions to the Archive Format
    return S_OK;
  }
  else if (item.LinkFlag > '7' || (item.LinkFlag < '0' && item.LinkFlag != 0))
    return S_FALSE;
  return S_OK;
}

}}
