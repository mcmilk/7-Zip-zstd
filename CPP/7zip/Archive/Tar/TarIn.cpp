// Archive/TarIn.cpp

#include "StdAfx.h"

#include "TarIn.h"
#include "TarHeader.h"

#include "Common/StringToInt.h"
#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"

namespace NArchive {
namespace NTar {
 
HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 &processedSize)
{
  RINOK(ReadStream(m_Stream, data, size, &processedSize));
  m_Position += processedSize;
  return S_OK;
}

HRESULT CInArchive::Open(IInStream *inStream)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  m_Stream = inStream;
  return S_OK;
}

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

static bool IsRecordLast(const char *record)
{
  for (int i = 0; i < NFileHeader::kRecordSize; i++)
    if (record[i] != 0)
      return false;
  return true;
}

static void ReadString(const char *s, int size, AString &result)
{
  if (size > NFileHeader::kRecordSize)
    size = NFileHeader::kNameSize;
  char tempString[NFileHeader::kRecordSize + 1];
  MyStrNCpy(tempString, s, size);
  tempString[size] = '\0';
  result = tempString;
}

static char GetHex(Byte value)
{
  return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10)));
}

HRESULT CInArchive::GetNextItemReal(bool &filled, CItemEx &item)
{
  item.LongLinkSize = 0;
  // NFileHeader::CRecord record;
  char record[NFileHeader::kRecordSize];
  char *cur = record;

  filled = false;

  UInt32 processedSize;
  item.HeaderPosition = m_Position;
  RINOK(ReadBytes(record, NFileHeader::kRecordSize, processedSize));
  if (processedSize == 0 || 
      (processedSize == NFileHeader::kRecordSize && IsRecordLast(record)))
    return S_OK;
  if (processedSize < NFileHeader::kRecordSize)
    return S_FALSE;
  
  // NFileHeader::CHeader &header = record.Header;
  
  AString name;
  ReadString(cur, NFileHeader::kNameSize, name);
  cur += NFileHeader::kNameSize;

  item.Name.Empty();
  int i;
  for (i = 0; i < name.Length(); i++)
  {
    char c = name[i];
    if (((Byte)c) < 0x08)
    {
      return S_FALSE;
    }
    if (((Byte)c) < 0x20)
    {
      item.Name += '[';
      item.Name += GetHex((Byte)(((Byte)c) >> 4));
      item.Name += GetHex((Byte)(((Byte)c) & 0xF));
      item.Name += ']';
    }
    else
      item.Name += c;
  }

  RIF(OctalToNumber32(cur, 8, item.Mode));
  cur += 8;

  if (!OctalToNumber32(cur, 8, item.UID))
    item.UID = 0;
  cur += 8;

  if (!OctalToNumber32(cur, 8, item.GID))
    item.GID = 0;
  cur += 8;

  RIF(OctalToNumber(cur, 12, item.Size));
  cur += 12;

  RIF(OctalToNumber32(cur, 12, item.ModificationTime));
  cur += 12;
  
  UInt32 checkSum;
  RIF(OctalToNumber32(cur, 8, checkSum));
  memmove(cur, NFileHeader::kCheckSumBlanks, 8);
  cur += 8;

  item.LinkFlag = *cur++;

  ReadString(cur, NFileHeader::kNameSize, item.LinkName);
  cur += NFileHeader::kNameSize;

  memmove(item.Magic, cur, 8);
  cur += 8;

  ReadString(cur, NFileHeader::kUserNameSize, item.UserName);
  cur += NFileHeader::kUserNameSize;
  ReadString(cur, NFileHeader::kUserNameSize, item.GroupName);
  cur += NFileHeader::kUserNameSize;

  item.DeviceMajorDefined = (cur[0] != 0);
  RIF(OctalToNumber32(cur, 8, item.DeviceMajor));
  cur += 8;
 
  item.DeviceMinorDefined = (cur[0] != 0);
  RIF(OctalToNumber32(cur, 8, item.DeviceMinor));
  cur += 8;

  AString prefix;
  ReadString(cur, NFileHeader::kPrefixSize, prefix);
  cur += NFileHeader::kPrefixSize;
  if (!prefix.IsEmpty() && item.IsMagic())
    item.Name = prefix + AString('/') + item.Name;

  if (item.LinkFlag == NFileHeader::NLinkFlag::kLink)
    item.Size = 0;
 
  
  UInt32 checkSumReal = 0;
  for(i = 0; i < NFileHeader::kRecordSize; i++)
    checkSumReal += Byte(record[i]);
  
  if (checkSumReal != checkSum)
    return S_FALSE;

  filled = true;
  return S_OK;
}

HRESULT CInArchive::GetNextItem(bool &filled, CItemEx &item)
{
  RINOK(GetNextItemReal(filled, item));
  if (!filled)
    return S_OK;
  // GNUtar extension
  if (item.LinkFlag == 'L')
  {
    if (item.Name.Compare(NFileHeader::kLongLink) != 0)
      if (item.Name.Compare(NFileHeader::kLongLink2) != 0)
        return S_FALSE;
    UInt64 headerPosition = item.HeaderPosition;

    UInt32 processedSize;
    AString fullName;
    char *buffer = fullName.GetBuffer((UInt32)item.Size + 1);
    RINOK(ReadBytes(buffer, (UInt32)item.Size, processedSize));
    buffer[item.Size] = '\0';
    fullName.ReleaseBuffer();
    if (processedSize != item.Size)
      return S_FALSE;
    RINOK(Skeep((0 - item.Size) & 0x1FF));
    RINOK(GetNextItemReal(filled, item));
    item.Name = fullName;
    item.LongLinkSize = item.HeaderPosition - headerPosition;
    item.HeaderPosition = headerPosition;
  }
  else if (item.LinkFlag == 'g' || item.LinkFlag == 'x' || item.LinkFlag == 'X')
  {
    // pax Extended Header
    return S_OK;
  }
  else if (item.LinkFlag > '7' || (item.LinkFlag < '0' && item.LinkFlag != 0))
    return S_FALSE;
  return S_OK;
}

HRESULT CInArchive::Skeep(UInt64 numBytes)
{
  UInt64 newPostion;
  RINOK(m_Stream->Seek(numBytes, STREAM_SEEK_CUR, &newPostion));
  m_Position += numBytes;
  if (m_Position != newPostion)
    return E_FAIL;
  return S_OK;
}


HRESULT CInArchive::SkeepDataRecords(UInt64 dataSize)
{
  return Skeep((dataSize + 0x1FF) & (~((UInt64)0x1FF)));
}

}}
