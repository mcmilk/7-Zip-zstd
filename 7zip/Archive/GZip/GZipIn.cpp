// Archive/GZipIn.cpp

#include "StdAfx.h"

#include "GZipIn.h"

#include "Common/Defs.h"
#include "Common/MyCom.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NGZip {
 
HRESULT CInArchive::ReadBytes(IInStream *inStream, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  RINOK(inStream->Read(data, size, &realProcessedSize));
  m_Position += realProcessedSize;
  if(realProcessedSize != size)
    return S_FALSE;
  return S_OK;
}

HRESULT CInArchive::UpdateCRCBytes(IInStream *inStream, 
    UInt32 numBytesToSkeep, CCRC &crc)
{
  while (numBytesToSkeep > 0)
  {
    const UInt32 kBufferSize = (1 << 12);
    Byte buffer[kBufferSize];
    UInt32 currentSize = MyMin(numBytesToSkeep, kBufferSize);
    RINOK(ReadBytes(inStream, buffer, currentSize));
    crc.Update(buffer, currentSize);
    numBytesToSkeep -= currentSize;
  }
  return S_OK;
}

HRESULT CInArchive::ReadByte(IInStream *inStream, Byte &value)
{
  return ReadBytes(inStream, &value, 1);
}

HRESULT CInArchive::ReadUInt16(IInStream *inStream, UInt16 &value)
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

HRESULT CInArchive::ReadUInt32(IInStream *inStream, UInt32 &value)
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

HRESULT CInArchive::ReadZeroTerminatedString(IInStream *inStream, AString &resString)
{
  resString.Empty();
  while(true)
  {
    Byte c;
    RINOK(ReadByte(inStream, c));
    if (c == 0)
      return S_OK;
    resString += char(c);
  }
}

HRESULT CInArchive::ReadHeader(IInStream *inStream, CItemEx &item)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));

  m_Position = m_StreamStartPosition;

  // NFileHeader::CBlock fileHeader;

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
    item.ExtraPosition = m_Position;
    RINOK(ReadUInt16(inStream, item.ExtraFieldSize));
    crc.Update(&item.ExtraFieldSize, 2);
    RINOK(UpdateCRCBytes(inStream, item.ExtraFieldSize, crc));
  }
  item.Name.Empty();
  if (item.NameIsPresent())
    RINOK(ReadZeroTerminatedString(inStream, item.Name));
  AString comment;
  if (item.CommentIsPresent())
  {
    item.CommentPosition = m_Position;
    RINOK(ReadZeroTerminatedString(inStream, comment));
    item.CommentSize = comment.Length() + 1;
  }
  if (item.HeaderCRCIsPresent())
  {
    UInt16 headerCRC;
    RINOK(ReadUInt16(inStream, headerCRC));
    if (item.NameIsPresent())
    {
      crc.Update((const char *)item.Name, item.Name.Length());
      crc.UpdateByte(0);
    }
    if (item.CommentIsPresent())
    {
      crc.Update((const char *)comment, comment.Length());
      crc.UpdateByte(0);
    }
    if ((UInt16)crc.GetDigest() != headerCRC)
      return S_FALSE;
  }
  item.DataPosition = m_Position;
  // item.UnPackSize32 = 0;
  // item.PackSize = 0;
  /*
  UInt64 newPosition;
  RINOK(inStream->Seek(-8, STREAM_SEEK_END, &newPosition));
  item.PackSize = newPosition - item.DataPosition;
  */
  return S_OK;
}

HRESULT CInArchive::ReadPostHeader(IInStream *inStream, CItemEx &item)
{
  RINOK(ReadUInt32(inStream, item.FileCRC));
  return ReadUInt32(inStream, item.UnPackSize32);
}

}}
