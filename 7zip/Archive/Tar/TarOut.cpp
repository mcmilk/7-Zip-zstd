// Archive/TarOut.cpp

#include "StdAfx.h"

#include "TarOut.h"
#include "TarHeader.h"

#include "Windows/Defs.h"

namespace NArchive {
namespace NTar {

HRESULT COutArchive::WriteBytes(const void *buffer, UINT32 size)
{
  UINT32 processedSize;
  RINOK(m_Stream->Write(buffer, size, &processedSize));
  if(processedSize != size)
    return E_FAIL;
  return S_OK;
}

void COutArchive::Create(ISequentialOutStream *outStream)
{
  m_Stream = outStream;
}

static AString MakeOctalString(UINT64 value)
{
  char s[32];
  _ui64toa(value, s, 8);
  return AString(s) + ' ';
}

static bool MakeOctalString8(char *s, UINT32 value)
{
  AString tempString = MakeOctalString(value);

  const kMaxSize = 8;
  if (tempString.Length() >= kMaxSize)
    return false;
  int numSpaces = kMaxSize - (tempString.Length() + 1);
  for(int i = 0; i < numSpaces; i++)
    s[i] = ' ';
  strcpy(s + numSpaces, tempString);
  return true;
}

static bool MakeOctalString12(char *s, UINT64 value)
{
  AString tempString  = MakeOctalString(value);
  const kMaxSize = 12;
  if (tempString.Length() > kMaxSize)
    return false;
  int numSpaces = kMaxSize - tempString.Length();
  for(int i = 0; i < numSpaces; i++)
    s[i] = ' ';
  memmove(s + numSpaces, (const char *)tempString, tempString.Length());
  return true;
}

static bool CopyString(char *dest, const AString &src, int maxSize)
{
  if (src.Length() >= maxSize)
    return false;
  strcpy(dest, src);
  return true;
}

#define RETURN_IF_NOT_TRUE(x) { if (!(x)) return E_FAIL; }

HRESULT COutArchive::WriteHeaderReal(const CItem &item)
{
  NFileHeader::CRecord record;
  for (int i = 0; i < NFileHeader::kRecordSize; i++)
    record.Padding[i] = 0;

  NFileHeader::CHeader &header = record.Header;

  // RETURN_IF_NOT_TRUE(CopyString(header.Name, item.Name, NFileHeader::kNameSize));
  if (item.Name.Length() > NFileHeader::kNameSize)
    return E_FAIL;
  strncpy(header.Name, item.Name, NFileHeader::kNameSize);

  RETURN_IF_NOT_TRUE(CopyString(header.LinkName, item.LinkName, NFileHeader::kNameSize));
  RETURN_IF_NOT_TRUE(CopyString(header.UserName, item.UserName, NFileHeader::kUserNameSize));
  RETURN_IF_NOT_TRUE(CopyString(header.GroupName, item.GroupName, NFileHeader::kGroupNameSize));

  RETURN_IF_NOT_TRUE(MakeOctalString8(header.Mode, item.Mode));
  RETURN_IF_NOT_TRUE(MakeOctalString8(header.UID, item.UID));
  RETURN_IF_NOT_TRUE(MakeOctalString8(header.GID, item.GID));

  RETURN_IF_NOT_TRUE(MakeOctalString12(header.Size, item.Size));
  RETURN_IF_NOT_TRUE(MakeOctalString12(header.ModificationTime, item.ModificationTime));
  header.LinkFlag = item.LinkFlag;
  memmove(header.Magic, item.Magic, 8);

  if (item.DeviceMajorDefined)
    RETURN_IF_NOT_TRUE(MakeOctalString8(header.DeviceMajor, item.DeviceMajor));
  if (item.DeviceMinorDefined)
    RETURN_IF_NOT_TRUE(MakeOctalString8(header.DeviceMinor, item.DeviceMinor));

  memmove(header.CheckSum, NFileHeader::kCheckSumBlanks, 8);

  UINT32 checkSumReal = 0;
  for(i = 0; i < NFileHeader::kRecordSize; i++)
    checkSumReal += BYTE(record.Padding[i]);

  RETURN_IF_NOT_TRUE(MakeOctalString8(header.CheckSum, checkSumReal));

  return WriteBytes(&record, sizeof(record));
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

HRESULT COutArchive::FillDataResidual(UINT64 dataSize)
{
  UINT32 lastRecordSize = UINT32(dataSize & (NFileHeader::kRecordSize - 1));
  if (lastRecordSize == 0)
    return S_OK;
  UINT32 residualSize = NFileHeader::kRecordSize - lastRecordSize;
  BYTE residualBytes[NFileHeader::kRecordSize];
  for (UINT32 i = 0; i < residualSize; i++)
    residualBytes[i] = 0;
  return WriteBytes(residualBytes, residualSize);
}

HRESULT COutArchive::WriteFinishHeader()
{
  NFileHeader::CRecord record;
  for (int i = 0; i < NFileHeader::kRecordSize; i++)
    record.Padding[i] = 0;
  return WriteBytes(&record, sizeof(record));
}

}}
