// Archive/GZip/InEngine.cpp

#include "StdAfx.h"

#include "InEngine.h"
#include "Windows/Defs.h"

#include "Common/Defs.h"

namespace NArchive {
namespace NGZip {
 
HRESULT CInArchive::ReadBytes(IInStream *aStream, void *aData, UINT32 aSize)
{
  UINT32 aProcessedSizeReal;
  RETURN_IF_NOT_S_OK(aStream->Read(aData, aSize, &aProcessedSizeReal));
  m_Position += aProcessedSizeReal;
  if(aProcessedSizeReal != aSize)
    return S_FALSE;
  return S_OK;
}

HRESULT CInArchive::UpdateCRCBytes(IInStream *aStream, 
    UINT32 anNumBytesToSkeep, CCRC &aCRC)
{
  while (anNumBytesToSkeep > 0)
  {
    const UINT32 kBufferSize = (1 << 12);
    BYTE aBuffer[kBufferSize];
    UINT32 aCurrentSize = MyMin(anNumBytesToSkeep, kBufferSize);
    RETURN_IF_NOT_S_OK(ReadBytes(aStream, aBuffer, aCurrentSize));
    aCRC.Update(aBuffer, aCurrentSize);
    anNumBytesToSkeep -= aCurrentSize;
  }
  return S_OK;
}

HRESULT CInArchive::ReadZeroTerminatedString(IInStream *aStream, AString &aString)
{
  aString.Empty();
  while(true)
  {
    char aChar;
    RETURN_IF_NOT_S_OK(ReadBytes(aStream, &aChar, sizeof(aChar)));
    if (aChar == 0)
      return S_OK;
    aString += aChar;
  }
}

HRESULT CInArchive::ReadHeader(IInStream *aStream, CItemInfoEx &anItemInfo)
{
  RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));

  m_Position = m_StreamStartPosition;
  NFileHeader::CBlock aFileHeader;

  RETURN_IF_NOT_S_OK(ReadBytes(aStream, &aFileHeader, sizeof(aFileHeader)));

  if (aFileHeader.Id != kSignature)
    return S_FALSE;

  anItemInfo.CompressionMethod = aFileHeader.CompressionMethod;
  anItemInfo.Flags = aFileHeader.Flags;
  anItemInfo.Time = aFileHeader.Time;
  anItemInfo.ExtraFlags = aFileHeader.ExtraFlags;
  anItemInfo.HostOS = aFileHeader.HostOS;

  CCRC aCRC;
  aCRC.Update(&aFileHeader, sizeof(aFileHeader));
  if (anItemInfo.ExtraFieldIsPresent())
  {
    anItemInfo.ExtraPosition = m_Position;
    RETURN_IF_NOT_S_OK(ReadBytes(aStream, &anItemInfo.ExtraFieldSize, 
        sizeof(anItemInfo.ExtraFieldSize)));
    aCRC.Update(&anItemInfo.ExtraFieldSize, sizeof(anItemInfo.ExtraFieldSize));
    RETURN_IF_NOT_S_OK(UpdateCRCBytes(aStream, anItemInfo.ExtraFieldSize, aCRC));
  }
  anItemInfo.Name.Empty();
  if (anItemInfo.NameIsPresent())
    RETURN_IF_NOT_S_OK(ReadZeroTerminatedString(aStream, anItemInfo.Name));
  AString aComment;
  if (anItemInfo.CommentIsPresent())
  {
    anItemInfo.CommentPosition = m_Position;
    RETURN_IF_NOT_S_OK(ReadZeroTerminatedString(aStream, aComment));
    anItemInfo.CommentSize = aComment.Length() + 1;
  }
  if (anItemInfo.HeaderCRCIsPresent())
  {
    UINT16 aHeaderCRC;
    RETURN_IF_NOT_S_OK(ReadBytes(aStream, &aHeaderCRC, sizeof(aHeaderCRC)));
    if (anItemInfo.NameIsPresent())
    {
      aCRC.Update((const char *)anItemInfo.Name, anItemInfo.Name.Length());
      BYTE aZeroByte = 0;;
      aCRC.Update(&aZeroByte, sizeof(aZeroByte));
    }
    if (anItemInfo.CommentIsPresent())
    {
      aCRC.Update((const char *)aComment, aComment.Length());
      BYTE aZeroByte = 0;;
      aCRC.Update(&aZeroByte, sizeof(aZeroByte));
    }
    if ((UINT16)aCRC.GetDigest() != aHeaderCRC)
      return S_FALSE;
  }

  anItemInfo.DataPosition = m_Position;

  anItemInfo.UnPackSize32 = 0;
  anItemInfo.PackSize = 0;

  /*
  UINT64 aNewPosition;
  RETURN_IF_NOT_S_OK(aStream->Seek(-8, STREAM_SEEK_END, &aNewPosition));
  anItemInfo.PackSize = aNewPosition - anItemInfo.DataPosition;

  RETURN_IF_NOT_S_OK(ReadBytes(aStream, &anItemInfo.FileCRC, sizeof(anItemInfo.FileCRC)));
  RETURN_IF_NOT_S_OK(ReadBytes(aStream, &anItemInfo.UnPackSize32, sizeof(anItemInfo.UnPackSize32)));
  */
  return S_OK;
}

HRESULT CInArchive::ReadPostInfo(IInStream *aStream, UINT32 &aCRC, UINT32 &anUnpackSize32)
{
  RETURN_IF_NOT_S_OK(ReadBytes(aStream, &aCRC, sizeof(aCRC)));
  return ReadBytes(aStream, &anUnpackSize32, sizeof(anUnpackSize32));
}

}}
