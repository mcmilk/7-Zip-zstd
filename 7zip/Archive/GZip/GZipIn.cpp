// Archive/GZipIn.cpp

#include "StdAfx.h"

#include "GZipIn.h"

#include "Common/Defs.h"
#include "Common/MyCom.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NGZip {
 
HRESULT CInArchive::ReadBytes(IInStream *inStream, void *data, UINT32 size)
{
  UINT32 realProcessedSize;
  RINOK(inStream->Read(data, size, &realProcessedSize));
  m_Position += realProcessedSize;
  if(realProcessedSize != size)
    return S_FALSE;
  return S_OK;
}

HRESULT CInArchive::UpdateCRCBytes(IInStream *inStream, 
    UINT32 numBytesToSkeep, CCRC &crc)
{
  while (numBytesToSkeep > 0)
  {
    const UINT32 kBufferSize = (1 << 12);
    BYTE buffer[kBufferSize];
    UINT32 currentSize = MyMin(numBytesToSkeep, kBufferSize);
    RINOK(ReadBytes(inStream, buffer, currentSize));
    crc.Update(buffer, currentSize);
    numBytesToSkeep -= currentSize;
  }
  return S_OK;
}

HRESULT CInArchive::ReadZeroTerminatedString(IInStream *inStream, AString &resString)
{
  resString.Empty();
  while(true)
  {
    char c;
    RINOK(ReadBytes(inStream, &c, sizeof(c)));
    if (c == 0)
      return S_OK;
    resString += c;
  }
}

HRESULT CInArchive::ReadHeader(IInStream *inStream, CItemEx &item)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));

  m_Position = m_StreamStartPosition;
  NFileHeader::CBlock fileHeader;

  RINOK(ReadBytes(inStream, &fileHeader, sizeof(fileHeader)));

  if (fileHeader.Id != kSignature)
    return S_FALSE;

  item.CompressionMethod = fileHeader.CompressionMethod;
  item.Flags = fileHeader.Flags;
  item.Time = fileHeader.Time;
  item.ExtraFlags = fileHeader.ExtraFlags;
  item.HostOS = fileHeader.HostOS;

  CCRC crc;
  crc.Update(&fileHeader, sizeof(fileHeader));
  if (item.ExtraFieldIsPresent())
  {
    item.ExtraPosition = m_Position;
    RINOK(ReadBytes(inStream, &item.ExtraFieldSize, 
        sizeof(item.ExtraFieldSize)));
    crc.Update(&item.ExtraFieldSize, sizeof(item.ExtraFieldSize));
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
    UINT16 headerCRC;
    RINOK(ReadBytes(inStream, &headerCRC, sizeof(headerCRC)));
    if (item.NameIsPresent())
    {
      crc.Update((const char *)item.Name, item.Name.Length());
      BYTE zeroByte = 0;;
      crc.Update(&zeroByte, sizeof(zeroByte));
    }
    if (item.CommentIsPresent())
    {
      crc.Update((const char *)comment, comment.Length());
      BYTE zeroByte = 0;;
      crc.Update(&zeroByte, sizeof(zeroByte));
    }
    if ((UINT16)crc.GetDigest() != headerCRC)
      return S_FALSE;
  }

  item.DataPosition = m_Position;

  item.UnPackSize32 = 0;
  item.PackSize = 0;

  /*
  UINT64 newPosition;
  RINOK(inStream->Seek(-8, STREAM_SEEK_END, &newPosition));
  item.PackSize = newPosition - item.DataPosition;

  RINOK(ReadBytes(inStream, &item.FileCRC, sizeof(item.FileCRC)));
  RINOK(ReadBytes(inStream, &item.UnPackSize32, sizeof(item.UnPackSize32)));
  */
  return S_OK;
}

HRESULT CInArchive::ReadPostInfo(IInStream *inStream, UINT32 &crc, UINT32 &unpackSize32)
{
  RINOK(ReadBytes(inStream, &crc, sizeof(crc)));
  return ReadBytes(inStream, &unpackSize32, sizeof(unpackSize32));
}

}}
