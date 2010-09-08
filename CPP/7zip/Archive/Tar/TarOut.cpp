// TarOut.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"

#include "../../Common/StreamUtils.h"

#include "TarOut.h"

namespace NArchive {
namespace NTar {

HRESULT COutArchive::WriteBytes(const void *buffer, UInt32 size)
{
  return WriteStream(m_Stream, buffer, size);
}

void COutArchive::Create(ISequentialOutStream *outStream)
{
  m_Stream = outStream;
}

static AString MakeOctalString(UInt64 value)
{
  char s[32];
  ConvertUInt64ToString(value, s, 8);
  return AString(s) + ' ';
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

static bool MakeOctalString8(char *s, UInt32 value)
{
  AString tempString = MakeOctalString(value);

  const int kMaxSize = 8;
  if (tempString.Length() >= kMaxSize)
    return false;
  int numSpaces = kMaxSize - (tempString.Length() + 1);
  for(int i = 0; i < numSpaces; i++)
    s[i] = ' ';
  MyStringCopy(s + numSpaces, (const char *)tempString);
  return true;
}

static void MakeOctalString12(char *s, UInt64 value)
{
  AString tempString  = MakeOctalString(value);
  const int kMaxSize = 12;
  if (tempString.Length() > kMaxSize)
  {
    // GNU extension;
    s[0] = (char)(Byte)0x80;
    s[1] = s[2] = s[3] = 0;
    for (int i = 0; i < 8; i++, value <<= 8)
      s[4 + i] = (char)(value >> 56);
    return;
  }
  int numSpaces = kMaxSize - tempString.Length();
  for(int i = 0; i < numSpaces; i++)
    s[i] = ' ';
  memmove(s + numSpaces, (const char *)tempString, tempString.Length());
}

static bool CopyString(char *dest, const AString &src, int maxSize)
{
  if (src.Length() >= maxSize)
    return false;
  MyStringCopy(dest, (const char *)src);
  return true;
}

#define RETURN_IF_NOT_TRUE(x) { if (!(x)) return E_FAIL; }

HRESULT COutArchive::WriteHeaderReal(const CItem &item)
{
  char record[NFileHeader::kRecordSize];
  char *cur = record;
  int i;
  for (i = 0; i < NFileHeader::kRecordSize; i++)
    record[i] = 0;

  // RETURN_IF_NOT_TRUE(CopyString(header.Name, item.Name, NFileHeader::kNameSize));
  if (item.Name.Length() > NFileHeader::kNameSize)
    return E_FAIL;
  MyStrNCpy(cur, item.Name, NFileHeader::kNameSize);
  cur += NFileHeader::kNameSize;

  RETURN_IF_NOT_TRUE(MakeOctalString8(cur, item.Mode)); cur += 8;
  RETURN_IF_NOT_TRUE(MakeOctalString8(cur, item.UID)); cur += 8;
  RETURN_IF_NOT_TRUE(MakeOctalString8(cur, item.GID)); cur += 8;

  MakeOctalString12(cur, item.Size); cur += 12;
  MakeOctalString12(cur, item.MTime); cur += 12;
  
  memmove(cur, NFileHeader::kCheckSumBlanks, 8);
  cur += 8;

  *cur++ = item.LinkFlag;

  RETURN_IF_NOT_TRUE(CopyString(cur, item.LinkName, NFileHeader::kNameSize));
  cur += NFileHeader::kNameSize;

  memmove(cur, item.Magic, 8);
  cur += 8;

  RETURN_IF_NOT_TRUE(CopyString(cur, item.User, NFileHeader::kUserNameSize));
  cur += NFileHeader::kUserNameSize;
  RETURN_IF_NOT_TRUE(CopyString(cur, item.Group, NFileHeader::kGroupNameSize));
  cur += NFileHeader::kGroupNameSize;


  if (item.DeviceMajorDefined)
    RETURN_IF_NOT_TRUE(MakeOctalString8(cur, item.DeviceMajor));
  cur += 8;

  if (item.DeviceMinorDefined)
    RETURN_IF_NOT_TRUE(MakeOctalString8(cur, item.DeviceMinor));
  cur += 8;


  UInt32 checkSumReal = 0;
  for(i = 0; i < NFileHeader::kRecordSize; i++)
    checkSumReal += Byte(record[i]);

  RETURN_IF_NOT_TRUE(MakeOctalString8(record + 148, checkSumReal));

  return WriteBytes(record, NFileHeader::kRecordSize);
}

HRESULT COutArchive::WriteHeader(const CItem &item)
{
  int nameSize = item.Name.Length();
  if (nameSize < NFileHeader::kNameSize)
    return WriteHeaderReal(item);

  CItem modifiedItem = item;
  int nameStreamSize = nameSize + 1;
  modifiedItem.Size = nameStreamSize;
  modifiedItem.LinkFlag = 'L';
  modifiedItem.Name = NFileHeader::kLongLink;
  modifiedItem.LinkName.Empty();
  RINOK(WriteHeaderReal(modifiedItem));
  RINOK(WriteBytes(item.Name, nameStreamSize));
  RINOK(FillDataResidual(nameStreamSize));

  modifiedItem = item;
  modifiedItem.Name = item.Name.Left(NFileHeader::kNameSize - 1);
  return WriteHeaderReal(modifiedItem);
}

HRESULT COutArchive::FillDataResidual(UInt64 dataSize)
{
  UInt32 lastRecordSize = UInt32(dataSize & (NFileHeader::kRecordSize - 1));
  if (lastRecordSize == 0)
    return S_OK;
  UInt32 residualSize = NFileHeader::kRecordSize - lastRecordSize;
  Byte residualBytes[NFileHeader::kRecordSize];
  for (UInt32 i = 0; i < residualSize; i++)
    residualBytes[i] = 0;
  return WriteBytes(residualBytes, residualSize);
}

HRESULT COutArchive::WriteFinishHeader()
{
  Byte record[NFileHeader::kRecordSize];
  int i;
  for (i = 0; i < NFileHeader::kRecordSize; i++)
    record[i] = 0;
  for (i = 0; i < 2; i++)
  {
    RINOK(WriteBytes(record, NFileHeader::kRecordSize));
  }
  return S_OK;
}

}}
