// Archive/cpioIn.cpp

#include "StdAfx.h"

#include "CpioIn.h"

#include "Windows/Defs.h"

#include "CpioHeader.h"

namespace NArchive {
namespace NCpio {
 
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

static bool HexStringToNumber(const char *s, UINT32 &resultValue)
{
  resultValue = 0;
  for (int i = 0; i < 8; i++)
  {
    char c = s[i];
    int d;
    if (c >= '0' && c <= '9')
      d = c - '0';
    else if (c >= 'A' && c <= 'F')
      d = 10 + c - 'A';
    else if (c >= 'a' && c <= 'f')
      d = 10 + c - 'a';
    else
      return false;
    resultValue *= 0x10;
    resultValue += d;
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

HRESULT CInArchive::GetNextItem(bool &filled, CItemEx &item)
{
  union
  {
    NFileHeader::CRecord record;
    NFileHeader::CRecord2 record2;
  };
  filled = false;

  UINT32 processedSize;
  item.HeaderPosition = m_Position;

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

    item.inode = ConvertValue(record2.c_ino, oldBE);
    item.Mode = ConvertValue(record2.c_mode, oldBE);
    item.UID = ConvertValue(record2.c_uid, oldBE);
    item.GID = ConvertValue(record2.c_gid, oldBE);
    item.Size = 
        (UINT32(ConvertValue(record2.c_filesizes[0], oldBE)) << 16) 
        + ConvertValue(record2.c_filesizes[1], oldBE);
    item.ModificationTime = 
       (UINT32(ConvertValue(record2.c_mtimes[0], oldBE)) << 16) + 
       ConvertValue(record2.c_mtimes[1], oldBE);
    item.NumLinks = ConvertValue(record2.c_nlink, oldBE);
    item.DevMajor = 0;
    item.DevMinor = ConvertValue(record2.c_dev, oldBE);
    item.RDevMajor =0;
    item.RDevMinor = ConvertValue(record2.c_rdev, oldBE);
    item.ChkSum = 0;
    nameSize = ConvertValue(record2.c_namesize, oldBE);
    item.HeaderSize = 
        (((nameSize + sizeof(NFileHeader::CRecord2) - 1) / 2) + 1) * 2; /* 4 byte padding for ("new cpio header" + "filename") */
        // nameSize + sizeof(NFileHeader::CRecord2);
    nameSize = item.HeaderSize - sizeof(NFileHeader::CRecord2);  
    item.OldHeader = true;
  }
  else
  {
    RINOK(ReadBytes((BYTE *)(&record) + 2, sizeof(record) - 2, processedSize));
    if (processedSize != sizeof(record) - 2)
      return S_FALSE;
    
    if (!record.CheckMagic())
      return S_FALSE;
    
    GetFromHex(record.inode, item.inode);
    GetFromHex(record.Mode, item.Mode);
    GetFromHex(record.UID, item.UID);
    GetFromHex(record.GID, item.GID);
    GetFromHex(record.nlink, item.NumLinks);
    GetFromHex(record.mtime, *(UINT32 *)&item.ModificationTime);
    GetFromHex(record.Size, item.Size);
    GetFromHex(record.DevMajor, item.DevMajor);
    GetFromHex(record.DevMinor, item.DevMinor);
    GetFromHex(record.RDevMajor, item.RDevMajor);
    GetFromHex(record.RDevMinor, item.RDevMinor);
    GetFromHex(record.ChkSum, item.ChkSum);
    GetFromHex(record.NameSize, nameSize)
    item.HeaderSize = 
        (((nameSize + sizeof(NFileHeader::CRecord) - 1) / 4) + 1) * 4; /* 4 byte padding for ("new cpio header" + "filename") */
    nameSize = item.HeaderSize - sizeof(NFileHeader::CRecord);  
    item.OldHeader = false;
  }
  if (nameSize == 0 || nameSize >= (1 << 27))
    return E_FAIL;
  RINOK(ReadBytes(item.Name.GetBuffer(nameSize), 
      nameSize, processedSize));
  if (processedSize != nameSize)
    return E_FAIL;
  item.Name.ReleaseBuffer();
  if (item.Name == NFileHeader::NMagic::kEndName)
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
