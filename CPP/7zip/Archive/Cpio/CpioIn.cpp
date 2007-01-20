// Archive/cpioIn.cpp

#include "StdAfx.h"

#include "CpioIn.h"

#include "Common/StringToInt.h"
#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"

#include "CpioHeader.h"

namespace NArchive {
namespace NCpio {
 
HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 &processedSize)
{
  RINOK(ReadStream(m_Stream, data, size, &processedSize));
  m_Position += processedSize;
  return S_OK;
}

Byte CInArchive::ReadByte()
{
  if (_blockPos >= _blockSize)
    throw "Incorrect cpio archive";
  return _block[_blockPos++];
}

UInt16 CInArchive::ReadUInt16()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = ReadByte();
    value |= (UInt16(b) << (8 * i));
  }
  return value;
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b = ReadByte();
    value |= (UInt32(b) << (8 * i));
  }
  return value;
}

HRESULT CInArchive::Open(IInStream *inStream)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  m_Stream = inStream;
  return S_OK;
}

bool CInArchive::ReadNumber(UInt32 &resultValue)
{
  resultValue = 0;
  for (int i = 0; i < 8; i++)
  {
    char c = char(ReadByte());
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

static bool OctalToNumber(const char *s, UInt64 &res)
{
  const char *end;
  res = ConvertOctStringToUInt64(s, &end);
  return (*end == ' ' || *end == 0);
}

static bool OctalToNumber32(const char *s, UInt32 &res)
{
  UInt64 res64;
  if (!OctalToNumber(s, res64))
    return false;
  res = (UInt32)res64;
  return (res64 <= 0xFFFFFFFF);
}

bool CInArchive::ReadOctNumber(int size, UInt32 &resultValue)
{
  char sz[32 + 4];
  int i;
  for (i = 0; i < size && i < 32; i++)
    sz[i] = (char)ReadByte();
  sz[i] = 0;
  return OctalToNumber32(sz, resultValue);
}

#define GetFromHex(y) { if (!ReadNumber(y)) return S_FALSE; }
#define GetFromOct6(y) { if (!ReadOctNumber(6, y)) return S_FALSE; }
#define GetFromOct11(y) { if (!ReadOctNumber(11, y)) return S_FALSE; }

static unsigned short ConvertValue(unsigned short value, bool convert)
{
  if (!convert)
    return value;
  return (unsigned short)((((unsigned short)(value & 0xFF)) << 8) | (value >> 8));
}

static UInt32 GetAlignedSize(UInt32 size, UInt32 align)
{
  while ((size & (align - 1)) != 0)
    size++;
  return size;
}


HRESULT CInArchive::GetNextItem(bool &filled, CItemEx &item)
{
  filled = false;

  UInt32 processedSize;
  item.HeaderPosition = m_Position;

  _blockSize = kMaxBlockSize;
  RINOK(ReadBytes(_block, 2, processedSize));
  if (processedSize != 2)
    return S_FALSE;
  _blockPos = 0;

  UInt32 nameSize;

  bool oldBE = 
    _block[0] == NFileHeader::NMagic::kMagicForRecord2[1] &&
    _block[1] == NFileHeader::NMagic::kMagicForRecord2[0];

  bool binMode = (_block[0] == NFileHeader::NMagic::kMagicForRecord2[0] &&
    _block[1] == NFileHeader::NMagic::kMagicForRecord2[1]) || 
    oldBE;

  if (binMode)
  {
    RINOK(ReadBytes(_block + 2, NFileHeader::kRecord2Size - 2, processedSize));
    if (processedSize != NFileHeader::kRecord2Size - 2)
      return S_FALSE;
    item.Align = 2;
    _blockPos = 2;
    item.DevMajor = 0;
    item.DevMinor = ConvertValue(ReadUInt16(), oldBE);
    item.inode = ConvertValue(ReadUInt16(), oldBE);
    item.Mode = ConvertValue(ReadUInt16(), oldBE);
    item.UID = ConvertValue(ReadUInt16(), oldBE);
    item.GID = ConvertValue(ReadUInt16(), oldBE);
    item.NumLinks = ConvertValue(ReadUInt16(), oldBE);
    item.RDevMajor =0;
    item.RDevMinor = ConvertValue(ReadUInt16(), oldBE);
    UInt16 timeHigh = ConvertValue(ReadUInt16(), oldBE);
    UInt16 timeLow = ConvertValue(ReadUInt16(), oldBE);
    item.ModificationTime = (UInt32(timeHigh) << 16) + timeLow;
    nameSize = ConvertValue(ReadUInt16(), oldBE);
    UInt16 sizeHigh = ConvertValue(ReadUInt16(), oldBE);
    UInt16 sizeLow = ConvertValue(ReadUInt16(), oldBE);
    item.Size = (UInt32(sizeHigh) << 16) + sizeLow;

    item.ChkSum = 0;
    item.HeaderSize = GetAlignedSize(
        nameSize + NFileHeader::kRecord2Size, item.Align);
    nameSize = item.HeaderSize - NFileHeader::kRecord2Size;  
  }
  else
  {
    RINOK(ReadBytes(_block + 2, 4, processedSize));
    if (processedSize != 4)
      return S_FALSE;

    bool magicOK = 
        memcmp(_block, NFileHeader::NMagic::kMagic1, 6) == 0 || 
        memcmp(_block, NFileHeader::NMagic::kMagic2, 6) == 0;
    _blockPos = 6;
    if (magicOK)
    {
      RINOK(ReadBytes(_block + 6, NFileHeader::kRecordSize - 6, processedSize));
      if (processedSize != NFileHeader::kRecordSize - 6)
        return S_FALSE;
      item.Align = 4;

      GetFromHex(item.inode);
      GetFromHex(item.Mode);
      GetFromHex(item.UID);
      GetFromHex(item.GID);
      GetFromHex(item.NumLinks);
      UInt32 modificationTime;
      GetFromHex(modificationTime);
      item.ModificationTime = modificationTime;
      GetFromHex(item.Size);
      GetFromHex(item.DevMajor);
      GetFromHex(item.DevMinor);
      GetFromHex(item.RDevMajor);
      GetFromHex(item.RDevMinor);
      GetFromHex(nameSize);
      GetFromHex(item.ChkSum);
      item.HeaderSize = GetAlignedSize(
          nameSize + NFileHeader::kRecordSize, item.Align);
      nameSize = item.HeaderSize - NFileHeader::kRecordSize;  
    }
    else
    {
      if (!memcmp(_block, NFileHeader::NMagic::kMagic3, 6) == 0)
        return S_FALSE;
      RINOK(ReadBytes(_block + 6, NFileHeader::kOctRecordSize - 6, processedSize));
      if (processedSize != NFileHeader::kOctRecordSize - 6)
        return S_FALSE;
      item.Align = 1;
      item.DevMajor = 0;
      GetFromOct6(item.DevMinor);
      GetFromOct6(item.inode);
      GetFromOct6(item.Mode);
      GetFromOct6(item.UID);
      GetFromOct6(item.GID);
      GetFromOct6(item.NumLinks);
      item.RDevMajor = 0;
      GetFromOct6(item.RDevMinor);
      UInt32 modificationTime;
      GetFromOct11(modificationTime);
      item.ModificationTime = modificationTime;
      GetFromOct6(nameSize);
      GetFromOct11(item.Size);  // ?????
      item.HeaderSize = GetAlignedSize(
          nameSize + NFileHeader::kOctRecordSize, item.Align);
      nameSize = item.HeaderSize - NFileHeader::kOctRecordSize;  
    }
  }
  if (nameSize == 0 || nameSize >= (1 << 27))
    return E_FAIL;
  RINOK(ReadBytes(item.Name.GetBuffer(nameSize), 
      nameSize, processedSize));
  if (processedSize != nameSize)
    return E_FAIL;
  item.Name.ReleaseBuffer();
  if (strcmp(item.Name, NFileHeader::NMagic::kEndName) == 0)
    return S_OK;
  filled = true;
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

HRESULT CInArchive::SkeepDataRecords(UInt64 dataSize, UInt32 align)
{
  while ((dataSize & (align - 1)) != 0)
    dataSize++;
  return Skeep(dataSize);
}

}}
