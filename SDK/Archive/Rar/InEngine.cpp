// Archive/Rar/InEngine.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/CRC.h"

#include "Archive/Rar/InEngine.h"
#include "Interface/LimitedStreams.h"

namespace NArchive {
namespace NRar {
 
static const char kEndOfString = '\0';
  
CInArchiveException::CInArchiveException(CCauseType aCause):
  Cause(aCause)
{}

void CInArchive::ThrowExceptionWithCode(
    CInArchiveException::CCauseType anCause)
{
  throw CInArchiveException(anCause);
}

bool CInArchive::Open(IInStream *aStream, const UINT64 *aSearchHeaderSizeLimit)
{
  if(aStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition) != S_OK)
    return false;
  m_Position = m_StreamStartPosition;
  m_Stream = aStream;
  if (ReadMarkerAndArchiveHeader(aSearchHeaderSizeLimit))
    return true;
  m_Stream.Release();
  return false;
}

void CInArchive::Close()
{
  m_Stream.Release();
}


static inline bool TestMarkerCandidate(const void *aTestBytes)
{
  // return (memcmp(aTestBytes, NHeader::kMarker, NHeader::kMarkerSize) == 0);
  for (UINT32 i = 0; i < NHeader::kMarkerSize; i++)
    if (((const BYTE *)aTestBytes)[i] != NHeader::kMarker[i])
      return false;
  return true;
}

bool CInArchive::FindAndReadMarker(const UINT64 *aSearchHeaderSizeLimit)
{
  // if (m_Length < NHeader::kMarkerSize)
  //   return false;
  m_ArchiveStartPosition = 0;
  m_Position = m_StreamStartPosition;
  if(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL) != S_OK)
    return false;

  BYTE aMarker[NHeader::kMarkerSize];
  UINT32 aProcessedSize; 
  ReadBytes(aMarker, NHeader::kMarkerSize, &aProcessedSize);
  if(aProcessedSize != NHeader::kMarkerSize)
    return false;
  if (TestMarkerCandidate(aMarker))
    return true;

  CByteDynamicBuffer aDynamicBuffer;
  static const UINT32 kSearchMarkerBufferSize = 0x10000;
  aDynamicBuffer.EnsureCapacity(kSearchMarkerBufferSize);
  BYTE *aBuffer = aDynamicBuffer;
  UINT32 aNumBytesPrev = NHeader::kMarkerSize - 1;
  memmove(aBuffer, aMarker + 1, aNumBytesPrev);
  UINT64 aCurTestPos = m_StreamStartPosition + 1;
  while(true)
  {
    if (aSearchHeaderSizeLimit != NULL)
      if (aCurTestPos - m_StreamStartPosition > *aSearchHeaderSizeLimit)
        return false;
    UINT32 aNumReadBytes = kSearchMarkerBufferSize - aNumBytesPrev;
    ReadBytes(aBuffer + aNumBytesPrev, aNumReadBytes, &aProcessedSize);
    UINT32 aNumBytesInBuffer = aNumBytesPrev + aProcessedSize;
    if (aNumBytesInBuffer < NHeader::kMarkerSize)
      return false;
    UINT32 aNumTests = aNumBytesInBuffer - NHeader::kMarkerSize + 1;
    for(UINT32 aPos = 0; aPos < aNumTests; aPos++, aCurTestPos++)
    { 
      if (TestMarkerCandidate(aBuffer + aPos))
      {
        m_ArchiveStartPosition = aCurTestPos;
        m_Position = aCurTestPos + NHeader::kMarkerSize;
        if(m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL) != S_OK)
          return false;
        return true;
      }
    }
    aNumBytesPrev = aNumBytesInBuffer - aNumTests;
    memmove(aBuffer, aBuffer + aNumTests, aNumBytesPrev);
  }
  return false;
}

//bool boolIsItCorrectArchive;

void CInArchive::ThrowUnexpectedEndOfArchiveException()
{
  ThrowExceptionWithCode(CInArchiveException::kUnexpectedEndOfArchive);
}

bool CInArchive::ReadBytesAndTestSize(void *aData, UINT32 aSize)
{
  UINT32 aProcessedSize;
  m_Stream->Read(aData, aSize, &aProcessedSize);
  return (aProcessedSize == aSize);
}

void CInArchive::ReadBytesAndTestResult(void *aData, UINT32 aSize)
{
  if(!ReadBytesAndTestSize(aData,aSize))
    ThrowUnexpectedEndOfArchiveException();
}

HRESULT CInArchive::ReadBytes(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->Read(aData, aSize, &aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  AddToSeekValue(aProcessedSizeReal);
  return aResult;
}

bool CInArchive::ReadMarkerAndArchiveHeader(const UINT64 *aSearchHeaderSizeLimit)
{
  if (!FindAndReadMarker(aSearchHeaderSizeLimit))
    return false;

  UINT32 aProcessedSize;
  ReadBytes(&m_ArchiveHeader, sizeof(m_ArchiveHeader), &aProcessedSize);
  if (aProcessedSize != sizeof(m_ArchiveHeader))
    return false;
  
  if(m_ArchiveHeader.CRC != m_ArchiveHeader.GetRealCRC())
    ThrowExceptionWithCode(CInArchiveException::kArchiveHeaderCRCError);
  if (m_ArchiveHeader.Type != NHeader::NBlockType::kArchiveHeader)
    return false;
  m_ArchiveCommentPosition = m_Position;
  m_SeekOnArchiveComment = true;
  return true;
}

void CInArchive::SkipArchiveComment()
{
  if (!m_SeekOnArchiveComment)
    return;
  AddToSeekValue(m_ArchiveHeader.Size - sizeof(m_ArchiveHeader));
  m_SeekOnArchiveComment = false;
}

void CInArchive::GetArchiveInfo(CInArchiveInfo &anArchiveInfo) const
{
  anArchiveInfo.StartPosition = m_ArchiveStartPosition;
  anArchiveInfo.Flags = m_ArchiveHeader.Flags;
  anArchiveInfo.CommentPosition = m_ArchiveCommentPosition;
  anArchiveInfo.CommentSize = m_ArchiveHeader.Size - sizeof(m_ArchiveHeader);
}

static void DecodeUnicodeFileName(const char *name, const BYTE *encName, 
    int encSize, wchar_t *unicodeName, int maxDecSize)
{
  int encPos = 0;
  int decPos = 0;
  int flagBits = 0;
  BYTE flags = 0;
  BYTE highByte = encName[encPos++];
  while (encPos < encSize && decPos < maxDecSize)
  {
    if (flagBits == 0)
    {
      flags = encName[encPos++];
      flagBits = 8;
    }
    switch(flags >> 6)
    {
      case 0:
        unicodeName[decPos++] = encName[encPos++];
        break;
      case 1:
        unicodeName[decPos++] = encName[encPos++] + (highByte << 8);
        break;
      case 2:
        unicodeName[decPos++] = encName[encPos] + (encName[encPos + 1] << 8);
        encPos += 2;
        break;
      case 3:
        {
          int length = encName[encPos++];
          if (length & 0x80)
          {
            BYTE correction = encName[encPos++];
            for (length = (length & 0x7f) + 2; 
                length > 0 && decPos < maxDecSize; length--, decPos++)
              unicodeName[decPos] = ((name[decPos] + correction) & 0xff) + (highByte << 8);
          }
          else
            for (length += 2; length > 0 && decPos < maxDecSize; length--, decPos++)
              unicodeName[decPos] = name[decPos];
        }
        break;
    }
    flags <<= 2;
    flagBits -= 2;
  }
  unicodeName[decPos < maxDecSize ? decPos : maxDecSize - 1] = 0;
}

void CInArchive::ReadName(const BYTE *data, CItemInfoEx &itemInfo, int nameSize)
{
  UINT32 sizeFileName = nameSize;
  itemInfo.UnicodeName.Empty();
  if (sizeFileName > 0)
  {
    m_NameBuffer.EnsureCapacity(sizeFileName + 1);
    char *buffer = (char *)m_NameBuffer;

    memmove(buffer, data, sizeFileName);

    int mainLen;
    for (mainLen = 0; mainLen < sizeFileName; mainLen++)
      if (buffer[mainLen] == '\0')
        break;
    buffer[mainLen] = '\0';
    itemInfo.Name = buffer;

    UINT32 unicodeNameSizeMax = MyMin(sizeFileName, UINT32(0x400));
    _unicodeNameBuffer.EnsureCapacity(unicodeNameSizeMax + 1);

    if((m_BlockHeader.Flags & NHeader::NFile::kUnicodeName) != 0  && 
        mainLen < sizeFileName)
    {
      DecodeUnicodeFileName(buffer, (const BYTE *)buffer + mainLen + 1, 
          sizeFileName - (mainLen + 1), _unicodeNameBuffer, unicodeNameSizeMax);
      itemInfo.UnicodeName = _unicodeNameBuffer;
    }
  }
  else
    itemInfo.Name.Empty();
}

static const BYTE *ReadTime(const BYTE *data, BYTE mask, CRarTime &rarTime)
{
  rarTime.LowSecond = ((mask & 4) != 0) ? 1 : 0;
  int numDigits = (mask & 3);
  rarTime.SubTime[0] = rarTime.SubTime[1] = rarTime.SubTime[2] = 0;
  for (int i = 0; i < numDigits; i++)
    rarTime.SubTime[3 - numDigits + i] = *data++;
  return data;
}

void CInArchive::ReadHeaderReal(const BYTE *data, CItemInfoEx &itemInfo)
{
  const BYTE *dataStart = data;
  const NHeader::NFile::CBlock &fileHeader = 
      *(const NHeader::NFile::CBlock *)data;
  // UINT32 sizeFileName = fileHeader.NameSize;
  
  itemInfo.Flags = m_BlockHeader.Flags; 
  itemInfo.PackSize = fileHeader.PackSize;
  itemInfo.UnPackSize = fileHeader.UnPackSize;
  itemInfo.HostOS = fileHeader.HostOS;
  itemInfo.FileCRC = fileHeader.FileCRC;
  itemInfo.LastWriteTime.DosTime = fileHeader.Time;
  itemInfo.LastWriteTime.LowSecond = 0;
  itemInfo.LastWriteTime.SubTime[0] = 
    itemInfo.LastWriteTime.SubTime[1] = 
    itemInfo.LastWriteTime.SubTime[2] = 0;


  itemInfo.UnPackVersion = fileHeader.UnPackVersion;
  itemInfo.Method = fileHeader.Method;
  itemInfo.Attributes = fileHeader.Attributes;
  
  data += sizeof(fileHeader);

  if((itemInfo.Flags & NHeader::NFile::kSize64Bits) != 0)
  {
    itemInfo.PackSize |= (*((const UINT64 *)data)) << 32;
    data += sizeof(UINT32);
    itemInfo.UnPackSize |= (*((const UINT64 *)data)) << 32;
    data += sizeof(UINT32);
  }

  ReadName(data, itemInfo, fileHeader.NameSize);
  data += fileHeader.NameSize;

  if (itemInfo.HasSalt())
  {
    memmove(itemInfo.Salt, data, sizeof(itemInfo.Salt));
    data += sizeof(itemInfo.Salt);
  }

  if (itemInfo.HasExtTime())
  {
    BYTE access = (*data) >> 4;
    data++;
    BYTE modif = (*data) >> 4;
    BYTE creat = (*data) & 0xF;
    data++;
    if ((modif & 8) != 0)
      data = ReadTime(data, modif, itemInfo.LastWriteTime);
    if (itemInfo.IsCreationTimeDefined = ((creat & 8) != 0))
    {
      itemInfo.CreationTime.DosTime = *(const UINT32 *)data;
      data += sizeof(UINT32);
      data = ReadTime(data, creat, itemInfo.CreationTime);
    }
    if (itemInfo.IsLastAccessTimeDefined = ((access & 8) != 0))
    {
      itemInfo.LastAccessTime.DosTime = *(const UINT32 *)data;
      data += sizeof(UINT32);
      data = ReadTime(data, access, itemInfo.LastAccessTime);
    }
  }

  UINT16 fileHeaderWithNameSize = 
    sizeof(NHeader::NBlock::CBlock) + data - dataStart;
  
  itemInfo.Position = m_Position;
  itemInfo.MainPartSize = fileHeaderWithNameSize;
  itemInfo.CommentSize = m_BlockHeader.HeadSize - fileHeaderWithNameSize;
  
  AddToSeekValue(m_BlockHeader.HeadSize);
}

void CInArchive::AddToSeekValue(UINT64 anAddValue)
{
  m_Position += anAddValue;
}

bool CInArchive::GetNextItem(CItemInfoEx &itemInfo)
{
  if ((m_ArchiveHeader.Flags & 
      NHeader::NArchive::kBlockHeadersAreEncrypted) != 0)
    return false;
  if (m_SeekOnArchiveComment)
    SkipArchiveComment();
  while (true)
  {
    if(!SeekInArchive(m_Position))
      return false;
    if(!ReadBytesAndTestSize(&m_BlockHeader, sizeof(m_BlockHeader)))
      return false;

    if (m_BlockHeader.HeadSize < 7)
      ThrowExceptionWithCode(CInArchiveException::kIncorrectArchive);

    if (m_BlockHeader.Type == NHeader::NBlockType::kFileHeader) 
    {
      m_FileHeaderData.EnsureCapacity(m_BlockHeader.HeadSize);
      UINT32 headerSize = m_BlockHeader.HeadSize - sizeof(m_BlockHeader);
      ReadBytesAndTestResult(m_FileHeaderData, headerSize);
      CCRC crc;
      crc.Update(&m_BlockHeader.Type, sizeof(m_BlockHeader) - 
          sizeof(m_BlockHeader.CRC));
          
      ReadHeaderReal(m_FileHeaderData, itemInfo); 

      crc.Update(m_FileHeaderData, headerSize - itemInfo.CommentSize);
      if ((crc.GetDigest() & 0xFFFF) != m_BlockHeader.CRC)
        ThrowExceptionWithCode(CInArchiveException::kFileHeaderCRCError);

      SeekInArchive(m_Position); // Move Position to compressed Data;    
      AddToSeekValue(itemInfo.PackSize);  // m_Position points Tto next header;
      return true;
    }
    if ((m_BlockHeader.Flags & NHeader::NBlock::kLongBlock) != 0)
    {
      UINT32 aDataSize;
      ReadBytesAndTestResult(&aDataSize, sizeof(aDataSize)); // test it
      AddToSeekValue(aDataSize);
    }
    AddToSeekValue(m_BlockHeader.HeadSize);
  }
}


void CInArchive::DirectGetBytes(void *aData, UINT32 aNum)
{  
  m_Stream->Read(aData, aNum, NULL);
}


bool CInArchive::SeekInArchive(UINT64 aPosition)
{
  UINT64 aNewPosition;
  m_Stream->Seek(aPosition, STREAM_SEEK_SET, &aNewPosition);
  return aNewPosition == aPosition;
}

ISequentialInStream* CInArchive::CreateLimitedStream(UINT64 aPosition, UINT64 aSize)
{
  CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
      CComObjectNoLock<CLimitedSequentialInStream>;
  CComPtr<ISequentialInStream> aStream(aStreamSpec);
  SeekInArchive(aPosition);
  aStreamSpec->Init(m_Stream, aSize);
  return aStream.Detach();
}

}}
