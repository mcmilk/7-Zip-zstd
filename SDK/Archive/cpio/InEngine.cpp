// Archive/cpio/InEngine.cpp

#include "StdAfx.h"

#include "InEngine.h"

#include "Windows/Defs.h"

#include "Archive/cpio/Header.h"

namespace NArchive {
namespace Ncpio {
 
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

static bool HexStringToNumber(const char *aString, UINT32 &aResultValue)
{
  aResultValue = 0;
  for (int i = 0; i < 8; i++)
  {
    char aChar = aString[i];
    int a;
    if (aChar >= '0' && aChar <= '9')
      a = aChar - '0';
    else if (aChar >= 'A' && aChar <= 'F')
      a = 10 + aChar - 'A';
    else if (aChar >= 'a' && aChar <= 'f')
      a = 10 + aChar - 'a';
    else
      return false;
    aResultValue *= 0x10;
    aResultValue += a;
  }
  return true;
}

#define GetFromHex(x, y) { if (!HexStringToNumber((x), (y))) return E_FAIL; }

HRESULT CInArchive::GetNextItem(bool &aFilled, CItemInfoEx &anItemInfo)
{
  NFileHeader::CRecord aRecord;
  aFilled = false;

  UINT32 aProcessedSize;
  anItemInfo.HeaderPosition = m_Position;
  RETURN_IF_NOT_S_OK(ReadBytes(&aRecord, sizeof(aRecord), aProcessedSize));
  if (aProcessedSize != sizeof(aRecord))
    return S_FALSE;

  if (!aRecord.CheckMagic())
    return S_FALSE;

  GetFromHex(aRecord.inode, anItemInfo.inode);
  GetFromHex(aRecord.Mode, anItemInfo.Mode);
  GetFromHex(aRecord.UID, anItemInfo.UID);
  GetFromHex(aRecord.GID, anItemInfo.GID);
  GetFromHex(aRecord.nlink, anItemInfo.NumLinks);
  GetFromHex(aRecord.mtime, *(UINT32 *)&anItemInfo.ModificationTime);
  GetFromHex(aRecord.Size, anItemInfo.Size);
  GetFromHex(aRecord.DevMajor, anItemInfo.DevMajor);
  GetFromHex(aRecord.DevMinor, anItemInfo.DevMinor);
  GetFromHex(aRecord.RDevMajor, anItemInfo.RDevMajor);
  GetFromHex(aRecord.RDevMinor, anItemInfo.RDevMinor);
  GetFromHex(aRecord.ChkSum, anItemInfo.ChkSum);
  UINT32 aNameSize;
  GetFromHex(aRecord.NameSize, aNameSize)
  if (aNameSize == 0 || aNameSize >= (1 << 27))
    return E_FAIL;
  anItemInfo.HeaderSize = 
      (((aNameSize + sizeof(NFileHeader::CRecord) - 1) / 4) + 1) * 4; /* 4 byte padding for ("new cpio header" + "filename") */
  aNameSize = anItemInfo.HeaderSize - sizeof(NFileHeader::CRecord);  
  RETURN_IF_NOT_S_OK(ReadBytes(anItemInfo.Name.GetBuffer(aNameSize), 
      aNameSize, aProcessedSize));
  if (aProcessedSize != aNameSize)
    return E_FAIL;
  anItemInfo.Name.ReleaseBuffer();
  if (anItemInfo.Name == NFileHeader::NMagic::kEndName)
    return S_OK;
  aFilled = true;
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
  return Skeep((aDataSize + 3) & 0xFFFFFFFFFFFFFFFC);
}

}}
