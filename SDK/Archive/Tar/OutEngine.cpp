// Archive/Tar/OutEngine.cpp

#include "StdAfx.h"

#include "OutEngine.h"

#include "Archive/Tar/Header.h"

#include "Windows/Defs.h"

namespace NArchive {
namespace NTar {

HRESULT COutArchive::WriteBytes(const void *aBuffer, UINT32 aSize)
{
  UINT32 aProcessedSize;
  RETURN_IF_NOT_S_OK(m_Stream->Write(aBuffer, aSize, &aProcessedSize));
  if(aProcessedSize != aSize)
    return E_FAIL;
  return S_OK;
}

void COutArchive::Create(ISequentialOutStream *aStream)
{
  m_Stream = aStream;
}

static AString MakeOctalString(UINT64 aValue)
{
  char aString[32];
  _ui64toa(aValue, aString, 8);
  return AString(aString) + ' ';
}

static bool MakeOctalString8(char *aString, UINT32 aValue)
{
  AString aTempString = MakeOctalString(aValue);

  const kMaxSize = 8;
  if (aTempString.Length() >= kMaxSize)
    return false;
  int aNumSpaces = kMaxSize - (aTempString.Length() + 1);
  for(int i = 0; i < aNumSpaces; i++)
    aString[i] = ' ';
  strcpy(aString + aNumSpaces, aTempString);
  return true;
}

static bool MakeOctalString12(char *aString, UINT64 aValue)
{
  AString aTempString  = MakeOctalString(aValue);
  const kMaxSize = 12;
  if (aTempString.Length() > kMaxSize)
    return false;
  int aNumSpaces = kMaxSize - aTempString.Length();
  for(int i = 0; i < aNumSpaces; i++)
    aString[i] = ' ';
  memmove(aString + aNumSpaces, (const char *)aTempString, aTempString.Length());
  return true;
}

static bool CopyString(char *aDest, const AString &aSrc, int aMaxSize)
{
  if (aSrc.Length() >= aMaxSize)
    return false;
  strcpy(aDest, aSrc);
  return true;
}

#define RETURN_IF_NOT_TRUE(x) { if (!(x)) return E_FAIL; }

HRESULT COutArchive::WriteHeaderReal(const CItemInfo &anItemInfo)
{
  NFileHeader::CRecord aRecord;
  for (int i = 0; i < NFileHeader::kRecordSize; i++)
    aRecord.Padding[i] = 0;

  NFileHeader::CHeader &aHeader = aRecord.Header;

  // RETURN_IF_NOT_TRUE(CopyString(aHeader.Name, anItemInfo.Name, NFileHeader::kNameSize));
  if (anItemInfo.Name.Length() > NFileHeader::kNameSize)
    return E_FAIL;
  strncpy(aHeader.Name, anItemInfo.Name, NFileHeader::kNameSize);

  RETURN_IF_NOT_TRUE(CopyString(aHeader.LinkName, anItemInfo.LinkName, NFileHeader::kNameSize));
  RETURN_IF_NOT_TRUE(CopyString(aHeader.UserName, anItemInfo.UserName, NFileHeader::kUserNameSize));
  RETURN_IF_NOT_TRUE(CopyString(aHeader.GroupName, anItemInfo.GroupName, NFileHeader::kGroupNameSize));

  RETURN_IF_NOT_TRUE(MakeOctalString8(aHeader.Mode, anItemInfo.Mode));
  RETURN_IF_NOT_TRUE(MakeOctalString8(aHeader.UID, anItemInfo.UID));
  RETURN_IF_NOT_TRUE(MakeOctalString8(aHeader.GID, anItemInfo.GID));

  RETURN_IF_NOT_TRUE(MakeOctalString12(aHeader.Size, anItemInfo.Size));
  RETURN_IF_NOT_TRUE(MakeOctalString12(aHeader.ModificationTime, anItemInfo.ModificationTime));
  aHeader.LinkFlag = anItemInfo.LinkFlag;
  memmove(aHeader.Magic, anItemInfo.Magic, 8);

  if (anItemInfo.DeviceMajorDefined)
    RETURN_IF_NOT_TRUE(MakeOctalString8(aHeader.DeviceMajor, anItemInfo.DeviceMajor));
  if (anItemInfo.DeviceMinorDefined)
    RETURN_IF_NOT_TRUE(MakeOctalString8(aHeader.DeviceMinor, anItemInfo.DeviceMinor));

  memmove(aHeader.CheckSum, NFileHeader::kCheckSumBlanks, 8);

  UINT32 aCheckSumReal = 0;
  for(i = 0; i < NFileHeader::kRecordSize; i++)
    aCheckSumReal += BYTE(aRecord.Padding[i]);

  RETURN_IF_NOT_TRUE(MakeOctalString8(aHeader.CheckSum, aCheckSumReal));

  return WriteBytes(&aRecord, sizeof(aRecord));
}

HRESULT COutArchive::WriteHeader(const CItemInfo &anItemInfo)
{
  int aNameSize = anItemInfo.Name.Length();
  if (aNameSize < NFileHeader::kNameSize)
    return WriteHeaderReal(anItemInfo);

  CItemInfo aModifiedItem = anItemInfo;
  int aNameStreamSize = aNameSize + 1;
  aModifiedItem.Size = aNameStreamSize;
  aModifiedItem.LinkFlag = 'L';
  aModifiedItem.Name = NFileHeader::kLongLink;
  aModifiedItem.LinkName.Empty();
  RETURN_IF_NOT_S_OK(WriteHeaderReal(aModifiedItem));
  RETURN_IF_NOT_S_OK(WriteBytes(anItemInfo.Name, aNameStreamSize));
  RETURN_IF_NOT_S_OK(FillDataResidual(aNameStreamSize));

  aModifiedItem = anItemInfo;
  aModifiedItem.Name = anItemInfo.Name.Left(NFileHeader::kNameSize - 1);
  return WriteHeaderReal(aModifiedItem);
}

HRESULT COutArchive::FillDataResidual(UINT64 aDataSize)
{
  UINT32 aLastRecordSize = UINT32(aDataSize & (NFileHeader::kRecordSize - 1));
  if (aLastRecordSize == 0)
    return S_OK;
  UINT32 aResidualSize = NFileHeader::kRecordSize - aLastRecordSize;
  BYTE aResidualBytes[NFileHeader::kRecordSize];
  for (UINT32 i = 0; i < aResidualSize; i++)
    aResidualBytes[i] = 0;
  return WriteBytes(aResidualBytes, aResidualSize);
}

HRESULT COutArchive::WriteFinishHeader()
{
  NFileHeader::CRecord aRecord;
  for (int i = 0; i < NFileHeader::kRecordSize; i++)
    aRecord.Padding[i] = 0;
  return WriteBytes(&aRecord, sizeof(aRecord));
}

}}
