// Archive/GZipIn.cpp

#include "StdAfx.h"

#include "GZipIn.h"

#include "Common/Defs.h"
#include "Common/MyCom.h"
#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"

extern "C" 
{ 
  #include "../../../../C/7zCrc.h" 
}

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

HRESULT CInArchive::ReadByte(ISequentialInStream *inStream, Byte &value, UInt32 &crc)
{
  RINOK(ReadBytes(inStream, &value, 1));
  crc = CRC_UPDATE_BYTE(crc, value);
  return S_OK;
}

HRESULT CInArchive::ReadUInt16(ISequentialInStream *inStream, UInt16 &value, UInt32 &crc)
{
  value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b;
    RINOK(ReadByte(inStream, b, crc));
    value |= (UInt16(b) << (8 * i));
  }
  return S_OK;
}

HRESULT CInArchive::ReadUInt32(ISequentialInStream *inStream, UInt32 &value, UInt32 &crc)
{
  value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b;
    RINOK(ReadByte(inStream, b, crc));
    value |= (UInt32(b) << (8 * i));
  }
  return S_OK;
}

HRESULT CInArchive::ReadZeroTerminatedString(ISequentialInStream *inStream, AString &resString, UInt32 &crc)
{
  resString.Empty();
  for (;;)
  {
    Byte c;
    RINOK(ReadByte(inStream, c, crc));
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
  UInt32 crc = CRC_INIT_VAL;;
  RINOK(ReadUInt16(inStream, signature, crc));
  if (signature != kSignature)
    return S_FALSE;
  
  RINOK(ReadByte(inStream, item.CompressionMethod, crc));
  RINOK(ReadByte(inStream, item.Flags, crc));
  RINOK(ReadUInt32(inStream, item.Time, crc));
  RINOK(ReadByte(inStream, item.ExtraFlags, crc));
  RINOK(ReadByte(inStream, item.HostOS, crc));
  
  if (item.ExtraFieldIsPresent())
  {
    UInt16 extraSize;
    RINOK(ReadUInt16(inStream, extraSize, crc));
    item.Extra.SetCapacity(extraSize);
    RINOK(ReadBytes(inStream, item.Extra, extraSize));
    crc = CrcUpdate(crc, item.Extra, extraSize);
  }
  if (item.NameIsPresent())
    RINOK(ReadZeroTerminatedString(inStream, item.Name, crc));
  if (item.CommentIsPresent())
    RINOK(ReadZeroTerminatedString(inStream, item.Comment, crc));
  if (item.HeaderCRCIsPresent())
  {
    UInt16 headerCRC;
    UInt32 dummy = 0;
    RINOK(ReadUInt16(inStream, headerCRC, dummy));
    if ((UInt16)CRC_GET_DIGEST(crc) != headerCRC)
      return S_FALSE;
  }
  return S_OK;
}

HRESULT CInArchive::ReadPostHeader(ISequentialInStream *inStream, CItem &item)
{
  UInt32 dummy = 0;
  RINOK(ReadUInt32(inStream, item.FileCRC, dummy));
  return ReadUInt32(inStream, item.UnPackSize32, dummy);
}

}}
