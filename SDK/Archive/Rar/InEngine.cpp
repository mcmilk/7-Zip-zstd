// Archive/Rar/InEngine.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
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

void CInArchive::ReadName(CItemInfoEx &anItemInfo)
{
  UINT32 aSizeFileName = m_FileHeader32.NameSize;
  anItemInfo.UnicodeName.Empty();
  if (aSizeFileName > 0)
  {
    m_NameBuffer.EnsureCapacity(aSizeFileName + 1);
    char *buffer = (char *)m_NameBuffer;

    ReadBytesAndTestResult(buffer, aSizeFileName);

    int mainLen;
    for (mainLen = 0; mainLen < aSizeFileName; mainLen++)
      if (buffer[mainLen] == '\0')
        break;
    buffer[mainLen] = '\0';
    anItemInfo.Name = buffer;

    UINT32 unicodeNameSizeMax = MyMin(aSizeFileName, UINT32(0x400));
    _unicodeNameBuffer.EnsureCapacity(unicodeNameSizeMax + 1);

    if((m_BlockHeader.Flags & NHeader::NFile::kUnicodeName) != 0  && 
        mainLen < aSizeFileName)
    {
      DecodeUnicodeFileName(buffer, (const BYTE *)buffer + mainLen + 1, 
          aSizeFileName - (mainLen + 1), _unicodeNameBuffer, unicodeNameSizeMax);
      anItemInfo.UnicodeName = _unicodeNameBuffer;
    }
  }
  else
    anItemInfo.Name.Empty();
}
void CInArchive::ReadHeader32Real(CItemInfoEx &anItemInfo)
{
  ReadBytesAndTestResult((BYTE*)(&m_FileHeader32) + sizeof(m_BlockHeader), 
    sizeof(m_FileHeader32) - sizeof(m_BlockHeader));
  UINT32 aSizeFileName = m_FileHeader32.NameSize;

  ReadName(anItemInfo);
  
  anItemInfo.Flags = m_FileHeader32.Flags; 
  anItemInfo.PackSize = m_FileHeader32.PackSize;
  anItemInfo.UnPackSize = m_FileHeader32.UnPackSize;
  anItemInfo.HostOS = m_FileHeader32.HostOS;
  anItemInfo.FileCRC = m_FileHeader32.FileCRC;
  anItemInfo.Time = m_FileHeader32.Time;
  anItemInfo.UnPackVersion = m_FileHeader32.UnPackVersion;
  anItemInfo.Method = m_FileHeader32.Method;
  anItemInfo.Attributes = m_FileHeader32.Attributes;
  
  UINT16 aFileHeaderWithNameSize = sizeof(m_FileHeader32) + aSizeFileName;
  
  anItemInfo.Position = m_Position;
  anItemInfo.MainPartSize = aFileHeaderWithNameSize;
  anItemInfo.CommentSize = m_FileHeader32.HeadSize - aFileHeaderWithNameSize;
  
  if (anItemInfo.HasSalt())
    ReadBytesAndTestResult(&anItemInfo.Salt, sizeof(anItemInfo.Salt));
  if(m_FileHeader32.HeadCRC != 
      m_FileHeader32.GetRealCRC(m_NameBuffer, aSizeFileName, 
          anItemInfo.HasSalt(), anItemInfo.Salt))
    ThrowExceptionWithCode(CInArchiveException::kFileHeaderCRCError);
  AddToSeekValue(m_FileHeader32.HeadSize);
}

void CInArchive::ReadHeader64Real(CItemInfoEx &anItemInfo)
{
  ReadBytesAndTestResult((BYTE*)(&m_FileHeader64) + sizeof(m_BlockHeader), 
    sizeof(m_FileHeader64) - sizeof(m_BlockHeader));
  UINT32 aSizeFileName = m_FileHeader64.NameSize;
  ReadName(anItemInfo);
  
  anItemInfo.Flags = m_FileHeader64.Flags; 
  anItemInfo.PackSize = (((UINT64)m_FileHeader64.PackSizeHigh) << 32) + m_FileHeader64.PackSizeLow;
  anItemInfo.UnPackSize = (((UINT64)m_FileHeader64.UnPackSizeHigh) << 32) + m_FileHeader64.UnPackSizeLow;
  anItemInfo.HostOS = m_FileHeader64.HostOS;
  anItemInfo.FileCRC = m_FileHeader64.FileCRC;
  anItemInfo.Time = m_FileHeader64.Time;
  anItemInfo.UnPackVersion = m_FileHeader64.UnPackVersion;
  anItemInfo.Method = m_FileHeader64.Method;
  anItemInfo.Attributes = m_FileHeader64.Attributes;
  
  UINT16 aFileHeaderWithNameSize = sizeof(m_FileHeader64) + aSizeFileName;
  
  anItemInfo.Position = m_Position;
  anItemInfo.MainPartSize = aFileHeaderWithNameSize;
  anItemInfo.CommentSize = m_FileHeader64.HeadSize - 
      aFileHeaderWithNameSize;
  
  if(m_FileHeader64.HeadCRC != 
      m_FileHeader64.GetRealCRC(m_NameBuffer, aSizeFileName))
    ThrowExceptionWithCode(CInArchiveException::kFileHeaderCRCError);
  AddToSeekValue(m_FileHeader64.HeadSize);
}

void CInArchive::AddToSeekValue(UINT64 anAddValue)
{
  m_Position += anAddValue;
}

bool CInArchive::GetNextItem(CItemInfoEx &anItemInfo)
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
      if((m_BlockHeader.Flags & NHeader::NFile::kSize64Bits) != 0)
        ReadHeader64Real(anItemInfo); 
      else
        ReadHeader32Real(anItemInfo); 
      SeekInArchive(m_Position);              // Move Position to compressed Data;    
      AddToSeekValue(anItemInfo.PackSize);  // m_Position points Tto next header;
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
