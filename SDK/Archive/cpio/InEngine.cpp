// Archive/cpio/InEngine.cpp

#include "StdAfx.h"

#include "InEngine.h"

#include "Windows/Defs.h"

#include "Archive/cpio/Header.h"

namespace NArchive {
namespace Ncpio {
 
HRESULT CInArchive::ReadBytes(void *aData, UINT32 aSize, UINT32 &processedSize)
{
  RINOK(m_Stream->Read(aData, aSize, &processedSize));
  m_Position += processedSize;
  return S_OK;
}

HRESULT CInArchive::Open(IInStream *aStream)
{
  RINOK(aStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
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

static inline unsigned short ConvertValue(
    unsigned short value, bool convert)
{
  if (!convert)
    return value;
  return (((unsigned short)(value & 0xFF)) << 8) | (value >> 8);
}

HRESULT CInArchive::GetNextItem(bool &filled, CItemInfoEx &itemInfo)
{
  union
  {
    NFileHeader::CRecord record;
    NFileHeader::CRecord2 record2;
  };
  filled = false;

  UINT32 processedSize;
  itemInfo.HeaderPosition = m_Position;

  RINOK(ReadBytes(&record2, 2, processedSize));
  if (processedSize != 2)
    return S_FALSE;

  UINT32 nameSize;

  unsigned short signature = *(unsigned short *)&record;
  bool oldBE = (signature == NFileHeader::NMagic::kMagicForRecord2BE);

  if (signature == NFileHeader::NMagic::kMagicForRecord2 || oldBE)
  {
    RINOK(ReadBytes((BYTE *)(&record2) + 2, 
        sizeof(record2) - 2, processedSize));
    if (processedSize != sizeof(record2) - 2)
      return S_FALSE;

    itemInfo.inode = ConvertValue(record2.c_ino, oldBE);
    itemInfo.Mode = ConvertValue(record2.c_mode, oldBE);
    itemInfo.UID = ConvertValue(record2.c_uid, oldBE);
    itemInfo.GID = ConvertValue(record2.c_gid, oldBE);
    itemInfo.Size = 
        (UINT32(ConvertValue(record2.c_filesizes[0], oldBE)) << 16) 
        + ConvertValue(record2.c_filesizes[1], oldBE);
    itemInfo.ModificationTime = 
       (UINT32(ConvertValue(record2.c_mtimes[0], oldBE)) << 16) + 
       ConvertValue(record2.c_mtimes[1], oldBE);
    itemInfo.NumLinks = ConvertValue(record2.c_nlink, oldBE);
    itemInfo.DevMajor = 0;
    itemInfo.DevMinor = ConvertValue(record2.c_dev, oldBE);
    itemInfo.RDevMajor =0;
    itemInfo.RDevMinor = ConvertValue(record2.c_rdev, oldBE);
    itemInfo.ChkSum = 0;
    nameSize = ConvertValue(record2.c_namesize, oldBE);
    itemInfo.HeaderSize = 
        (((nameSize + sizeof(NFileHeader::CRecord2) - 1) / 2) + 1) * 2; /* 4 byte padding for ("new cpio header" + "filename") */
        // nameSize + sizeof(NFileHeader::CRecord2);
    nameSize = itemInfo.HeaderSize - sizeof(NFileHeader::CRecord2);  
    itemInfo.OldHeader = true;
  }
  else
  {
    RINOK(ReadBytes((BYTE *)(&record) + 2, sizeof(record) - 2, processedSize));
    if (processedSize != sizeof(record) - 2)
      return S_FALSE;
    
    if (!record.CheckMagic())
      return S_FALSE;
    
    GetFromHex(record.inode, itemInfo.inode);
    GetFromHex(record.Mode, itemInfo.Mode);
    GetFromHex(record.UID, itemInfo.UID);
    GetFromHex(record.GID, itemInfo.GID);
    GetFromHex(record.nlink, itemInfo.NumLinks);
    GetFromHex(record.mtime, *(UINT32 *)&itemInfo.ModificationTime);
    GetFromHex(record.Size, itemInfo.Size);
    GetFromHex(record.DevMajor, itemInfo.DevMajor);
    GetFromHex(record.DevMinor, itemInfo.DevMinor);
    GetFromHex(record.RDevMajor, itemInfo.RDevMajor);
    GetFromHex(record.RDevMinor, itemInfo.RDevMinor);
    GetFromHex(record.ChkSum, itemInfo.ChkSum);
    GetFromHex(record.NameSize, nameSize)
    itemInfo.HeaderSize = 
        (((nameSize + sizeof(NFileHeader::CRecord) - 1) / 4) + 1) * 4; /* 4 byte padding for ("new cpio header" + "filename") */
    nameSize = itemInfo.HeaderSize - sizeof(NFileHeader::CRecord);  
    itemInfo.OldHeader = false;
  }
  if (nameSize == 0 || nameSize >= (1 << 27))
    return E_FAIL;
  RINOK(ReadBytes(itemInfo.Name.GetBuffer(nameSize), 
      nameSize, processedSize));
  if (processedSize != nameSize)
    return E_FAIL;
  itemInfo.Name.ReleaseBuffer();
  if (itemInfo.Name == NFileHeader::NMagic::kEndName)
    return S_OK;
  filled = true;
  return S_OK;
}

HRESULT CInArchive::Skeep(UINT64 aNumBytes)
{
  UINT64 aNewPostion;
  RINOK(m_Stream->Seek(aNumBytes, STREAM_SEEK_CUR, &aNewPostion));
  m_Position += aNumBytes;
  if (m_Position != aNewPostion)
    return E_FAIL;
  return S_OK;
}

HRESULT CInArchive::SkeepDataRecords(UINT64 aDataSize, bool OldHeader)
{
  if (OldHeader)
    return Skeep((aDataSize + 1) & 0xFFFFFFFFFFFFFFFE);
  return Skeep((aDataSize + 3) & 0xFFFFFFFFFFFFFFFC);
}

}}
