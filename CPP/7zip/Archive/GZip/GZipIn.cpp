// Archive/GZipIn.cpp

#include "StdAfx.h"

#include "GZipIn.h"

#include "Common/Defs.h"
#include "Common/MyCom.h"
#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"

namespace NArchive {
namespace NGZip {
 
HRESULT CInArchive::ReadBytes(ISequentialInStream *inStream, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  RINOK(ReadStream(inStream, data, size, &realProcessedSize));
  m_Position += realProcessedSize;
  if(realProcessedSize != size)
    return S_FALSE;
  return S_OK;
}

HRESULT CInArchive::ReadByte(ISequentialInStream *inStream, Byte &value)
{
  return ReadBytes(inStream, &value, 1);
}

HRESULT CInArchive::ReadUInt16(ISequentialInStream *inStream, UInt16 &value)
{
  value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b;
    RINOK(ReadByte(inStream, b));
    value |= (UInt16(b) << (8 * i));
  }
  return S_OK;
}

HRESULT CInArchive::ReadUInt32(ISequentialInStream *inStream, UInt32 &value)
{
  value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b;
    RINOK(ReadByte(inStream, b));
    value |= (UInt32(b) << (8 * i));
  }
  return S_OK;
}

HRESULT CInArchive::ReadZeroTerminatedString(ISequentialInStream *inStream, AString &resString, CCRC &crc)
{
  resString.Empty();
  for (;;)
  {
    Byte c;
    RINOK(ReadByte(inStream, c));
    crc.UpdateByte(c);
    if (c == 0)
      return S_OK;
    resString += char(c);
  }
}

HRESULT CInArchive::ReadHeader(ISequentialInStream *inStream, CItem &item)
{
  item.Clear();
  m_Position = 0;

  UInt16 signature;
  RINOK(ReadUInt16(inStream, signature));
  if (signature != kSignature)
    return S_FALSE;
  RINOK(ReadByte(inStream, item.CompressionMethod));
  RINOK(ReadByte(inStream, item.Flags));
  RINOK(ReadUInt32(inStream, item.Time));
  RINOK(ReadByte(inStream, item.ExtraFlags));
  RINOK(ReadByte(inStream, item.HostOS));
  
  CCRC crc;
  crc.Update(&signature, 2);
  crc.UpdateByte(item.CompressionMethod);
  crc.UpdateByte(item.Flags);
  crc.UpdateUInt32(item.Time);
  crc.UpdateByte(item.ExtraFlags);
  crc.UpdateByte(item.HostOS);

  if (item.ExtraFieldIsPresent())
  {
    UInt16 extraSize;
    RINOK(ReadUInt16(inStream, extraSize));
    crc.UpdateUInt16(extraSize);
    item.Extra.SetCapacity(extraSize);
    RINOK(ReadBytes(inStream, item.Extra, extraSize));
    crc.Update(item.Extra, extraSize);
  }
  if (item.NameIsPresent())
    RINOK(ReadZeroTerminatedString(inStream, item.Name, crc));
  if (item.CommentIsPresent())
    RINOK(ReadZeroTerminatedString(inStream, item.Comment, crc));
  if (item.HeaderCRCIsPresent())
  {
    UInt16 headerCRC;
    RINOK(ReadUInt16(inStream, headerCRC));
    if ((UInt16)crc.GetDigest() != headerCRC)
      return S_FALSE;
  }
  return S_OK;
}

HRESULT CInArchive::ReadPostHeader(ISequentialInStream *inStream, CItem &item)
{
  RINOK(ReadUInt32(inStream, item.FileCRC));
  return ReadUInt32(inStream, item.UnPackSize32);
}

}}
