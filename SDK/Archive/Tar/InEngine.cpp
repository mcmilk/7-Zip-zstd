// Archive/Tar/InEngine.cpp

#include "StdAfx.h"

#include "InEngine.h"

#include "Archive/Tar/Header.h"
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

HRESULT CInArchive::GetNextItemReal(bool &filled, CItemInfoEx &itemInfo)
{
  itemInfo.LongLinkSize = 0;
  NFileHeader::CRecord record;
  filled = false;

  UINT32 processedSize;
  itemInfo.HeaderPosition = m_Position;
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
  itemInfo.Name = tempString;

  for (int i = 0; i < itemInfo.Name.Length(); i++)
    if (((BYTE)itemInfo.Name[i]) < 0x20)
      return S_FALSE;
  itemInfo.LinkFlag = header.LinkFlag;

  BYTE linkFlag = itemInfo.LinkFlag;

  ReturnIfBadOctal(header.Mode, 8);
  ReturnIfBadOctal(header.UID, 8);
  ReturnIfBadOctal(header.GID, 8);
  ReturnIfBadOctal(header.Size, 12);
  ReturnIfBadOctal(header.ModificationTime, 12);
  ReturnIfBadOctal(header.CheckSum, 8);
  ReturnIfBadOctal(header.DeviceMajor, 8);
  ReturnIfBadOctal(header.DeviceMinor, 8);

  itemInfo.Mode = OctalToNumber(header.Mode);
  itemInfo.UID = OctalToNumber(header.UID);
  itemInfo.GID = OctalToNumber(header.GID);
  
  itemInfo.Size = OctalToNumber(header.Size);
  if (itemInfo.LinkFlag == NFileHeader::NLinkFlag::kLink)
    itemInfo.Size = 0;

  itemInfo.ModificationTime = OctalToNumber(header.ModificationTime);
  

  itemInfo.LinkName = header.LinkName;
  memmove(itemInfo.Magic, header.Magic, 8);

  itemInfo.UserName = header.UserName;
  itemInfo.GroupName = header.GroupName;


  itemInfo.DeviceMajorDefined = strlen(header.DeviceMajor) > 0;
  if (itemInfo.DeviceMajorDefined)
    itemInfo.DeviceMajor = OctalToNumber(header.DeviceMajor);
  
  itemInfo.DeviceMinorDefined = strlen(header.DeviceMinor) > 0;
  if (itemInfo.DeviceMinorDefined)
  itemInfo.DeviceMinor = OctalToNumber(header.DeviceMinor);
  
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

HRESULT CInArchive::GetNextItem(bool &filled, CItemInfoEx &itemInfo)
{
  RINOK(GetNextItemReal(filled, itemInfo));
  if (!filled)
    return S_OK;
  // GNUtar extension
  if (itemInfo.LinkFlag == 'L')
  {
    if (itemInfo.Name.Compare(NFileHeader::kLongLink) != 0)
      return S_FALSE;
    UINT64 headerPosition = itemInfo.HeaderPosition;

    UINT32 processedSize;
    AString fullName;
    char *buffer = fullName.GetBuffer(itemInfo.Size + 1);
    RINOK(ReadBytes(buffer, itemInfo.Size, processedSize));
    buffer[itemInfo.Size] = '\0';
    fullName.ReleaseBuffer();
    if (processedSize != itemInfo.Size)
      return S_FALSE;
    RINOK(Skeep((0 - itemInfo.Size) & 0x1FF));
    RINOK(GetNextItemReal(filled, itemInfo));
    itemInfo.Name = fullName;
    itemInfo.LongLinkSize = itemInfo.HeaderPosition - headerPosition;
    itemInfo.HeaderPosition = headerPosition;
  }
  else if (itemInfo.LinkFlag > '7' || (itemInfo.LinkFlag < '0' && itemInfo.LinkFlag != 0))
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
  return Skeep((dataSize + 511) & 0xFFFFFFFFFFFFFE00);
}

}}
