// Archive/RarIn.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/CRC.h"

#include "RarIn.h"
#include "../../Common/LimitedStreams.h"

namespace NArchive {
namespace NRar {
 
static const char kEndOfString = '\0';
  
void CInArchive::ThrowExceptionWithCode(
    CInArchiveException::CCauseType cause)
{
  throw CInArchiveException(cause);
}

bool CInArchive::Open(IInStream *inStream, const UINT64 *searchHeaderSizeLimit)
{
  if(inStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition) != S_OK)
    return false;
  m_Position = m_StreamStartPosition;
  m_Stream = inStream;
  if (ReadMarkerAndArchiveHeader(searchHeaderSizeLimit))
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

bool CInArchive::FindAndReadMarker(const UINT64 *searchHeaderSizeLimit)
{
  // if (m_Length < NHeader::kMarkerSize)
  //   return false;
  m_ArchiveStartPosition = 0;
  m_Position = m_StreamStartPosition;
  if(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL) != S_OK)
    return false;

  BYTE marker[NHeader::kMarkerSize];
  UINT32 processedSize; 
  ReadBytes(marker, NHeader::kMarkerSize, &processedSize);
  if(processedSize != NHeader::kMarkerSize)
    return false;
  if (TestMarkerCandidate(marker))
    return true;

  CByteDynamicBuffer dynamicBuffer;
  static const UINT32 kSearchMarkerBufferSize = 0x10000;
  dynamicBuffer.EnsureCapacity(kSearchMarkerBufferSize);
  BYTE *buffer = dynamicBuffer;
  UINT32 numBytesPrev = NHeader::kMarkerSize - 1;
  memmove(buffer, marker + 1, numBytesPrev);
  UINT64 curTestPos = m_StreamStartPosition + 1;
  while(true)
  {
    if (searchHeaderSizeLimit != NULL)
      if (curTestPos - m_StreamStartPosition > *searchHeaderSizeLimit)
        return false;
    UINT32 numReadBytes = kSearchMarkerBufferSize - numBytesPrev;
    ReadBytes(buffer + numBytesPrev, numReadBytes, &processedSize);
    UINT32 numBytesInBuffer = numBytesPrev + processedSize;
    if (numBytesInBuffer < NHeader::kMarkerSize)
      return false;
    UINT32 numTests = numBytesInBuffer - NHeader::kMarkerSize + 1;
    for(UINT32 pos = 0; pos < numTests; pos++, curTestPos++)
    { 
      if (TestMarkerCandidate(buffer + pos))
      {
        m_ArchiveStartPosition = curTestPos;
        m_Position = curTestPos + NHeader::kMarkerSize;
        if(m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL) != S_OK)
          return false;
        return true;
      }
    }
    numBytesPrev = numBytesInBuffer - numTests;
    memmove(buffer, buffer + numTests, numBytesPrev);
  }
  return false;
}

//bool boolIsItCorrectArchive;

void CInArchive::ThrowUnexpectedEndOfArchiveException()
{
  ThrowExceptionWithCode(CInArchiveException::kUnexpectedEndOfArchive);
}

bool CInArchive::ReadBytesAndTestSize(void *data, UINT32 size)
{
  UINT32 processedSize;
  m_Stream->Read(data, size, &processedSize);
  return (processedSize == size);
}

void CInArchive::ReadBytesAndTestResult(void *data, UINT32 size)
{
  if(!ReadBytesAndTestSize(data,size))
    ThrowUnexpectedEndOfArchiveException();
}

HRESULT CInArchive::ReadBytes(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = m_Stream->Read(data, size, &realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  AddToSeekValue(realProcessedSize);
  return result;
}

bool CInArchive::ReadMarkerAndArchiveHeader(const UINT64 *searchHeaderSizeLimit)
{
  if (!FindAndReadMarker(searchHeaderSizeLimit))
    return false;

  UINT32 processedSize;
  ReadBytes(&m_ArchiveHeader, sizeof(m_ArchiveHeader), &processedSize);
  if (processedSize != sizeof(m_ArchiveHeader))
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

void CInArchive::GetArchiveInfo(CInArchiveInfo &archiveInfo) const
{
  archiveInfo.StartPosition = m_ArchiveStartPosition;
  archiveInfo.Flags = m_ArchiveHeader.Flags;
  archiveInfo.CommentPosition = m_ArchiveCommentPosition;
  archiveInfo.CommentSize = m_ArchiveHeader.Size - sizeof(m_ArchiveHeader);
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

void CInArchive::ReadName(const BYTE *data, CItemEx &item, int nameSize)
{
  int sizeFileName = nameSize;
  item.UnicodeName.Empty();
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
    item.Name = buffer;

    int unicodeNameSizeMax = MyMin(sizeFileName, (0x400));
    _unicodeNameBuffer.EnsureCapacity(unicodeNameSizeMax + 1);

    if((m_BlockHeader.Flags & NHeader::NFile::kUnicodeName) != 0  && 
        mainLen < sizeFileName)
    {
      DecodeUnicodeFileName(buffer, (const BYTE *)buffer + mainLen + 1, 
          sizeFileName - (mainLen + 1), _unicodeNameBuffer, unicodeNameSizeMax);
      item.UnicodeName = _unicodeNameBuffer;
    }
  }
  else
    item.Name.Empty();
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

void CInArchive::ReadHeaderReal(const BYTE *data, CItemEx &item)
{
  const BYTE *dataStart = data;
  const NHeader::NFile::CBlock &fileHeader = 
      *(const NHeader::NFile::CBlock *)data;
  // UINT32 sizeFileName = fileHeader.NameSize;
  
  item.Flags = m_BlockHeader.Flags; 
  item.PackSize = fileHeader.PackSize;
  item.UnPackSize = fileHeader.UnPackSize;
  item.HostOS = fileHeader.HostOS;
  item.FileCRC = fileHeader.FileCRC;
  item.LastWriteTime.DosTime = fileHeader.Time;
  item.LastWriteTime.LowSecond = 0;
  item.LastWriteTime.SubTime[0] = 
    item.LastWriteTime.SubTime[1] = 
    item.LastWriteTime.SubTime[2] = 0;


  item.UnPackVersion = fileHeader.UnPackVersion;
  item.Method = fileHeader.Method;
  item.Attributes = fileHeader.Attributes;
  
  data += sizeof(fileHeader);

  if((item.Flags & NHeader::NFile::kSize64Bits) != 0)
  {
    item.PackSize |= (*((const UINT64 *)data)) << 32;
    data += sizeof(UINT32);
    item.UnPackSize |= (*((const UINT64 *)data)) << 32;
    data += sizeof(UINT32);
  }

  ReadName(data, item, fileHeader.NameSize);
  data += fileHeader.NameSize;

  if (item.HasSalt())
  {
    memmove(item.Salt, data, sizeof(item.Salt));
    data += sizeof(item.Salt);
  }

  if (item.HasExtTime())
  {
    BYTE access = (*data) >> 4;
    data++;
    BYTE modif = (*data) >> 4;
    BYTE creat = (*data) & 0xF;
    data++;
    if ((modif & 8) != 0)
      data = ReadTime(data, modif, item.LastWriteTime);
    if (item.IsCreationTimeDefined = ((creat & 8) != 0))
    {
      item.CreationTime.DosTime = *(const UINT32 *)data;
      data += sizeof(UINT32);
      data = ReadTime(data, creat, item.CreationTime);
    }
    if (item.IsLastAccessTimeDefined = ((access & 8) != 0))
    {
      item.LastAccessTime.DosTime = *(const UINT32 *)data;
      data += sizeof(UINT32);
      data = ReadTime(data, access, item.LastAccessTime);
    }
  }

  UINT16 fileHeaderWithNameSize = 
    sizeof(NHeader::NBlock::CBlock) + data - dataStart;
  
  item.Position = m_Position;
  item.MainPartSize = fileHeaderWithNameSize;
  item.CommentSize = m_BlockHeader.HeadSize - fileHeaderWithNameSize;
  
  AddToSeekValue(m_BlockHeader.HeadSize);
}

void CInArchive::AddToSeekValue(UINT64 addValue)
{
  m_Position += addValue;
}

bool CInArchive::GetNextItem(CItemEx &item)
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
          
      ReadHeaderReal(m_FileHeaderData, item); 

      crc.Update(m_FileHeaderData, headerSize - item.CommentSize);
      if ((crc.GetDigest() & 0xFFFF) != m_BlockHeader.CRC)
        ThrowExceptionWithCode(CInArchiveException::kFileHeaderCRCError);

      SeekInArchive(m_Position); // Move Position to compressed Data;    
      AddToSeekValue(item.PackSize);  // m_Position points Tto next header;
      return true;
    }
    if ((m_BlockHeader.Flags & NHeader::NBlock::kLongBlock) != 0)
    {
      UINT32 dataSize;
      ReadBytesAndTestResult(&dataSize, sizeof(dataSize)); // test it
      AddToSeekValue(dataSize);
    }
    AddToSeekValue(m_BlockHeader.HeadSize);
  }
}


void CInArchive::DirectGetBytes(void *data, UINT32 size)
{  
  m_Stream->Read(data, size, NULL);
}


bool CInArchive::SeekInArchive(UINT64 position)
{
  UINT64 newPosition;
  m_Stream->Seek(position, STREAM_SEEK_SET, &newPosition);
  return newPosition == position;
}

ISequentialInStream* CInArchive::CreateLimitedStream(UINT64 position, UINT64 size)
{
  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  SeekInArchive(position);
  streamSpec->Init(m_Stream, size);
  return inStream.Detach();
}

}}
