// Archive/arj/InEngine.cpp

#include "StdAfx.h"

#include "Windows/Defs.h"
#include "InEngine.h"
#include "Common/StringConvert.h"
#include "Common/Buffer.h"
#include "Common/CRC.h"

namespace NArchive {
namespace Narj {
 
// static const char kEndOfString = '\0';
  
CInArchiveException::CInArchiveException(CCauseType aCause):
  Cause(aCause)
{}

HRESULT CInArchive::ReadBytes(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->Read(aData, aSize, &aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  IncreasePositionValue(aProcessedSizeReal);
  return aResult;
}

inline bool TestMarkerCandidate(const void *aTestBytes, UINT32 aMaxSize)
{
  if (aMaxSize < 2 + 2 + 4)
    return false;
  const BYTE *aBlock = ((const BYTE *)(aTestBytes));
  if (aBlock[0] != NSignature::kSig0 || aBlock[1] != NSignature::kSig1)
    return false;
  UINT16 aBlockSize = *((const UINT16 *)(aBlock + 2));
  if (aMaxSize < 2 + 2 + aBlockSize + 4)
    return false;
  aBlock += 4;
  if (aBlockSize == 0 || aBlockSize > 2600)
    return false;
  UINT32 aCRCFromFile = *(const UINT32 *)(aBlock + aBlockSize);
  return (CCRC::VerifyDigest(aCRCFromFile, aBlock, aBlockSize));
}

bool CInArchive::FindAndReadMarker(const UINT64 *aSearchHeaderSizeLimit)
{
  // m_ArchiveInfo.StartPosition = 0;
  m_Position = m_StreamStartPosition;
  if(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL) != S_OK)
    return false;

  const kMarkerSizeMax = 2 + 2 + kMaxBlockSize + sizeof(UINT32);

  CByteBuffer aDynamicBuffer;
  static const UINT32 kSearchMarkerBufferSize = 0x10000;
  aDynamicBuffer.SetCapacity(kSearchMarkerBufferSize);
  BYTE *aBuffer = aDynamicBuffer;

  UINT32 aProcessedSize; 
  ReadBytes(aBuffer, 2 + 2 + kMaxBlockSize + sizeof(UINT32), &aProcessedSize);
  if (aProcessedSize == 0)
    return false;
  if (TestMarkerCandidate(aBuffer, aProcessedSize))
  {
    m_Position = m_StreamStartPosition;
    if(m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL) != S_OK)
      return false;
    return true;
  }

  UINT32 aNumBytesPrev = aProcessedSize - 1;
  memmove(aBuffer, aBuffer + 1, aNumBytesPrev);
  UINT64 aCurTestPos = m_StreamStartPosition + 1;
  while(true)
  {
    if (aSearchHeaderSizeLimit != NULL)
      if (aCurTestPos - m_StreamStartPosition > *aSearchHeaderSizeLimit)
        return false;
    UINT32 aNumReadBytes = kSearchMarkerBufferSize - aNumBytesPrev;
    ReadBytes(aBuffer + aNumBytesPrev, aNumReadBytes, &aProcessedSize);
    UINT32 aNumBytesInBuffer = aNumBytesPrev + aProcessedSize;
    if (aNumBytesInBuffer < 1)
      return false;
    UINT32 aNumTests = aNumBytesInBuffer;
    for(UINT32 aPos = 0; aPos < aNumTests; aPos++, aCurTestPos++)
    { 
      if (TestMarkerCandidate(aBuffer + aPos, aNumBytesInBuffer - aPos))
      {
        // m_ArchiveInfo.StartPosition = aCurTestPos;
        m_Position = aCurTestPos;
        if(m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL) != S_OK)
          return false;
        return true;
      }
    }
    aNumBytesPrev = aNumBytesInBuffer - aNumTests;
    memmove(aBuffer, aBuffer + aNumTests, aNumBytesPrev);
  }
}

void CInArchive::IncreasePositionValue(UINT64 anAddValue)
{
  m_Position += anAddValue;
}

void CInArchive::IncreaseRealPosition(UINT64 anAddValue)
{
  if(m_Stream->Seek(anAddValue, STREAM_SEEK_CUR, &m_Position) != S_OK)
    throw CInArchiveException(CInArchiveException::kSeekStreamError);
}

bool CInArchive::ReadBytesAndTestSize(void *aData, UINT32 aSize)
{
  UINT32 aProcessedSizeReal;
  if(ReadBytes(aData, aSize, &aProcessedSizeReal) != S_OK)
    throw CInArchiveException(CInArchiveException::kReadStreamError);
  return (aProcessedSizeReal == aSize);
}

void CInArchive::SafeReadBytes(void *aData, UINT32 aSize)
{
  if(!ReadBytesAndTestSize(aData, aSize))
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
}

bool CInArchive::ReadBlock()
{
  SafeReadBytes(&m_BlockSize, sizeof(m_BlockSize));
  if (m_BlockSize == 0)
    return false;
  SafeReadBytes(m_Block, m_BlockSize);
  UINT32 aCRCFromFile;
  ReadBytesAndTestSize(&aCRCFromFile, sizeof(aCRCFromFile));
  if (!CCRC::VerifyDigest(aCRCFromFile, m_Block, m_BlockSize))
    throw CInArchiveException(CInArchiveException::kCRCError);
  return true;
}

bool CInArchive::ReadBlock2()
{
  BYTE anID[2];
  ReadBytesAndTestSize(anID, 2);
  if (anID[0] != NSignature::kSig0 || anID[1] != NSignature::kSig1)
    throw CInArchiveException(CInArchiveException::kIncorrectArchive);
  return ReadBlock();
}

bool CInArchive::Open(IInStream *aStream, const UINT64 *aSearchHeaderSizeLimit)
{
  m_Stream = aStream;
  if(m_Stream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition) != S_OK)
    return false;
  m_Position = m_StreamStartPosition;
  if (!FindAndReadMarker(aSearchHeaderSizeLimit))
    return false;
  if (!ReadBlock2())
    return false;
  while(true)
    if (!ReadBlock())
      break;
  return true;
}

void CInArchive::Close()
{
  m_Stream.Release();
}

//////////////////////////////////////
// Read Operations

void CInArchive::ThrowIncorrectArchiveException()
{
  throw CInArchiveException(CInArchiveException::kIncorrectArchive);
}

HRESULT CInArchive::GetNextItem(bool &aFilled, CItemInfoEx &anItem)
{
  aFilled  = false;
  if (!ReadBlock2())
    return S_OK;

  const NFileHeader::CHeader &aHeader = *(const NFileHeader::CHeader *)m_Block;

  anItem.Version = aHeader.Version;
  anItem.ExtractVersion = aHeader.ExtractVersion;
  anItem.HostOS = aHeader.HostOS;
  anItem.Flags = aHeader.Flags;
  anItem.Method = aHeader.Method;
  anItem.FileType = aHeader.FileType;
  anItem.ModifiedTime = aHeader.ModifiedTime;
  anItem.PackSize = aHeader.PackSize;
  anItem.Size = aHeader.Size;
  anItem.FileCRC = aHeader.FileCRC;
  anItem.FileAccessMode = aHeader.FileAccessMode;

  /*
  UINT32 anExtraData;
  if ((aHeader.Flags & NFileHeader::NFlags::kExtFile) != 0)
  {
    anExtraData = *(const UINT32 *)(m_Block + aPos);
  }
  */
  int aPos = aHeader.FirstHeaderSize;

  for (; aPos < m_BlockSize; aPos++)
  {
    char aByte = m_Block[aPos];
    if (aByte == 0)
      break;
    anItem.Name += aByte;
  }

  while(true)
    if (!ReadBlock())
      break;

  anItem.DataPosition = m_Position;

  aFilled  = true;
  return S_OK;
}  

}}
