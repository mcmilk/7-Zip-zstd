// Archive/TarIn.cpp

#include "StdAfx.h"

#include "TarIn.h"
#include "TarHeader.h"

#include "Windows/Defs.h"

namespace NArchive {
namespace NTar {
 
HRESULT CInArchive::ReadBytes(void *data, UINT32 size, UINT32 &processedSize)
{
  RINOK(m_Stream->Read(data, size, &processedSize));
  m_Position += processedSize;
  return S_OK;
}

HRESULT CInArchive::Open(IInStream *inStream)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  m_Stream = inStream;
  return S_OK;
}

static UINT32 OctalToNumber(const char *srcString)
{
  char *endPtr;
  return(strtoul(srcString, &endPtr, 8));
}

static bool CheckOctalString(const char *srcString, int numChars)
{
  for(int i = 0; i < numChars; i++)
  {
    char c = srcString[i];
    if (c == 0)
      return true;
    if (c >= '0' && c <= '7')
      continue;
    if (c != ' ')
      return false;
  }
  return true;
}

#define ReturnIfBadOctal(x, y) { if (!CheckOctalString((x), (y))) return S_FALSE; }

static bool IsRecordLast(const NFileHeader::CRecord &record)
{
  for (int i = 0; i < sizeof(record); i++)
    if (record.Padding[i] != 0)
      return false;
  return true;
}

HRESULT CInArchive::GetNextItemReal(bool &filled, CItemEx &item)
{
  item.LongLinkSize = 0;
  NFileHeader::CRecord record;
  filled = false;

  UINT32 processedSize;
  item.HeaderPosition = m_Position;
  RINOK(ReadBytes(&record, sizeof(record), processedSize));
  if (processedSize == 0 || 
      (processedSize == sizeof(record) && IsRecordLast(record)))
    return S_OK;
  if (processedSize < sizeof(record))
    return S_FALSE;
  
  NFileHeader::CHeader &header = record.Header;
  
  char tempString[NFileHeader::kNameSize + 1];
  strncpy(tempString, header.Name, NFileHeader::kNameSize);
  tempString[NFileHeader::kNameSize] = '\0';
  item.Name = tempString;

  int i;
  for (i = 0; i < item.Name.Length(); i++)
    if (((BYTE)item.Name[i]) < 0x20)
      return S_FALSE;
  item.LinkFlag = header.LinkFlag;

  BYTE linkFlag = item.LinkFlag;

  ReturnIfBadOctal(header.Mode, 8);
  ReturnIfBadOctal(header.UID, 8);
  ReturnIfBadOctal(header.GID, 8);
  ReturnIfBadOctal(header.Size, 12);
  ReturnIfBadOctal(header.ModificationTime, 12);
  ReturnIfBadOctal(header.CheckSum, 8);
  ReturnIfBadOctal(header.DeviceMajor, 8);
  ReturnIfBadOctal(header.DeviceMinor, 8);

  item.Mode = OctalToNumber(header.Mode);
  item.UID = OctalToNumber(header.UID);
  item.GID = OctalToNumber(header.GID);
  
  item.Size = OctalToNumber(header.Size);
  if (item.LinkFlag == NFileHeader::NLinkFlag::kLink)
    item.Size = 0;

  item.ModificationTime = OctalToNumber(header.ModificationTime);
  

  item.LinkName = header.LinkName;
  memmove(item.Magic, header.Magic, 8);

  item.UserName = header.UserName;
  item.GroupName = header.GroupName;


  item.DeviceMajorDefined = (header.DeviceMajor[0] != 0);
  if (item.DeviceMajorDefined)
    item.DeviceMajor = OctalToNumber(header.DeviceMajor);
  
  item.DeviceMinorDefined = (header.DeviceMinor[0] != 0);
  if (item.DeviceMinorDefined)
  item.DeviceMinor = OctalToNumber(header.DeviceMinor);
  
  UINT32 checkSum = OctalToNumber(header.CheckSum);

  memmove(header.CheckSum, NFileHeader::kCheckSumBlanks, 8);

  UINT32 checkSumReal = 0;
  for(i = 0; i < NFileHeader::kRecordSize; i++)
    checkSumReal += BYTE(record.Padding[i]);
  
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
      return S_FALSE;
    UINT64 headerPosition = item.HeaderPosition;

    UINT32 processedSize;
    AString fullName;
    char *buffer = fullName.GetBuffer((UINT32)item.Size + 1);
    RINOK(ReadBytes(buffer, (UINT32)item.Size, processedSize));
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
  else if (item.LinkFlag > '7' || (item.LinkFlag < '0' && item.LinkFlag != 0))
    return S_FALSE;
  return S_OK;
}

HRESULT CInArchive::Skeep(UINT64 numBytes)
{
  UINT64 newPostion;
  RINOK(m_Stream->Seek(numBytes, STREAM_SEEK_CUR, &newPostion));
  m_Position += numBytes;
  if (m_Position != newPostion)
    return E_FAIL;
  return S_OK;
}


HRESULT CInArchive::SkeepDataRecords(UINT64 dataSize)
{
  return Skeep((dataSize + 511) & 
      #if ( __GNUC__)
      0xFFFFFFFFFFFFFE00LL
      #else
      0xFFFFFFFFFFFFFE00
      #endif
      );
}

}}
