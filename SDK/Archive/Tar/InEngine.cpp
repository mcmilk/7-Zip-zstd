// Archive/Tar/InEngine.cpp

#include "StdAfx.h"

#include "InEngine.h"

#include "Archive/Tar/Header.h"

namespace NArchive {
namespace NTar {
 
#define RETURN_IF_NOT_S_OK(x) { HRESULT aResult = (x); if(aResult != S_OK) return aResult; }

HRESULT CInArchive::ReadBytes(void *aData, UINT32 aSize, UINT32 &aProcessedSize)
{
  RETURN_IF_NOT_S_OK(m_Stream->Read(aData, aSize, &aProcessedSize));
  m_Position += aProcessedSize;
  return S_OK;
}

HRESULT CInArchive::Open(IInStream *aStream)
{
  RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  m_Stream = aStream;
  return S_OK;
}

static UINT32 OctalToNumber(const char *aString)
{
  char *anEndPtr;
  return(strtoul(aString, &anEndPtr, 8));
}

bool CheckOctalString(const char *aString, int aNumChars)
{
  for(int i = 0; i < aNumChars; i++)
  {
    char aChar = aString[i];
    if (aChar == 0)
      return true;
    if (aChar >= '0' && aChar <= '7')
      continue;
    if (aChar != ' ')
      return false;
  }
  return true;
}

#define ReturnIfBadOctal(x, y) { if (!CheckOctalString((x), (y))) return S_FALSE; }

static bool IsRecordLast(const NFileHeader::CRecord &aRecord)
{
  for (int i = 0; i < sizeof(aRecord); i++)
    if (aRecord.Padding[i] != 0)
      return false;
  return true;
}

HRESULT CInArchive::GetNextItemReal(bool &aFilled, CItemInfoEx &anItemInfo)
{
  anItemInfo.LongLinkSize = 0;
  NFileHeader::CRecord aRecord;
  aFilled = false;

  UINT32 aProcessedSize;
  anItemInfo.HeaderPosition = m_Position;
  RETURN_IF_NOT_S_OK(ReadBytes(&aRecord, sizeof(aRecord), aProcessedSize));
  if (aProcessedSize == 0 || 
      (aProcessedSize == sizeof(aRecord) && IsRecordLast(aRecord)))
    return S_OK;
  if (aProcessedSize < sizeof(aRecord))
    return S_FALSE;
  
  NFileHeader::CHeader &aHeader = aRecord.Header;
  
  char aTempString[NFileHeader::kNameSize + 1];
  strncpy(aTempString, aHeader.Name, NFileHeader::kNameSize);
  aTempString[NFileHeader::kNameSize] = '\0';
  anItemInfo.Name = aTempString;

  for (int i = 0; i < anItemInfo.Name.Length(); i++)
    if (((BYTE)anItemInfo.Name[i]) < 0x20)
      return S_FALSE;
  anItemInfo.LinkFlag = aHeader.LinkFlag;

  BYTE aLinkFlag = anItemInfo.LinkFlag;

  ReturnIfBadOctal(aHeader.Mode, 8);
  ReturnIfBadOctal(aHeader.UID, 8);
  ReturnIfBadOctal(aHeader.GID, 8);
  ReturnIfBadOctal(aHeader.Size, 12);
  ReturnIfBadOctal(aHeader.ModificationTime, 12);
  ReturnIfBadOctal(aHeader.CheckSum, 8);
  ReturnIfBadOctal(aHeader.DeviceMajor, 8);
  ReturnIfBadOctal(aHeader.DeviceMinor, 8);

  anItemInfo.Mode = OctalToNumber(aHeader.Mode);
  anItemInfo.UID = OctalToNumber(aHeader.UID);
  anItemInfo.GID = OctalToNumber(aHeader.GID);
  anItemInfo.Size = OctalToNumber(aHeader.Size);
  anItemInfo.ModificationTime = OctalToNumber(aHeader.ModificationTime);
  

  anItemInfo.LinkName = aHeader.LinkName;
  memmove(anItemInfo.Magic, aHeader.Magic, 8);

  anItemInfo.UserName = aHeader.UserName;
  anItemInfo.GroupName = aHeader.GroupName;


  anItemInfo.DeviceMajorDefined = strlen(aHeader.DeviceMajor) > 0;
  if (anItemInfo.DeviceMajorDefined)
    anItemInfo.DeviceMajor = OctalToNumber(aHeader.DeviceMajor);
  
  anItemInfo.DeviceMinorDefined = strlen(aHeader.DeviceMinor) > 0;
  if (anItemInfo.DeviceMinorDefined)
  anItemInfo.DeviceMinor = OctalToNumber(aHeader.DeviceMinor);
  
  UINT32 aCheckSum = OctalToNumber(aHeader.CheckSum);

  memmove(aHeader.CheckSum, NFileHeader::kCheckSumBlanks, 8);

  UINT32 aCheckSumReal = 0;
  for(i = 0; i < NFileHeader::kRecordSize; i++)
    aCheckSumReal += BYTE(aRecord.Padding[i]);
  
  if (aCheckSumReal != aCheckSum)
    return S_FALSE;


  aFilled = true;
  return S_OK;
}

HRESULT CInArchive::GetNextItem(bool &aFilled, CItemInfoEx &anItemInfo)
{
  RETURN_IF_NOT_S_OK(GetNextItemReal(aFilled, anItemInfo));
  if (!aFilled)
    return S_OK;
  // GNUtar extension
  if (anItemInfo.LinkFlag == 'L')
  {
    if (anItemInfo.Name.Compare(NFileHeader::kLongLink) != 0)
      return S_FALSE;
    UINT64 aHeaderPosition = anItemInfo.HeaderPosition;

    UINT32 aProcessedSize;
    AString aFullName;
    char *aString = aFullName.GetBuffer(anItemInfo.Size + 1);
    RETURN_IF_NOT_S_OK(ReadBytes(aString, anItemInfo.Size, aProcessedSize));
    aString[anItemInfo.Size] = '\0';
    aFullName.ReleaseBuffer();
    if (aProcessedSize != anItemInfo.Size)
      return S_FALSE;
    RETURN_IF_NOT_S_OK(Skeep((0 - anItemInfo.Size) & 0x1FF));
    RETURN_IF_NOT_S_OK(GetNextItemReal(aFilled, anItemInfo));
    anItemInfo.Name = aFullName;
    anItemInfo.LongLinkSize = anItemInfo.HeaderPosition - aHeaderPosition;
    anItemInfo.HeaderPosition = aHeaderPosition;
  }
  else if (anItemInfo.LinkFlag > '7' || (anItemInfo.LinkFlag < '0' && anItemInfo.LinkFlag != 0))
    return S_FALSE;
  return S_OK;
}




HRESULT CInArchive::Skeep(UINT64 aNumBytes)
{
  UINT64 aNewPostion;
  RETURN_IF_NOT_S_OK(m_Stream->Seek(aNumBytes, STREAM_SEEK_CUR, &aNewPostion));
  m_Position += aNumBytes;
  if (m_Position != aNewPostion)
    return E_FAIL;
  return S_OK;
}

HRESULT CInArchive::SkeepDataRecords(UINT64 aDataSize)
{
  return Skeep((aDataSize + 511) & 0xFFFFFFFFFFFFFE00);
}

}}
