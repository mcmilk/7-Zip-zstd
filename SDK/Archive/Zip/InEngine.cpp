// Archive/Zip/InEngine.cpp

#include "StdAfx.h"

#include "Windows/Defs.h"
#include "InEngine.h"
#include "Common/StringConvert.h"
#include "Interface/LimitedStreams.h"
#include "Common/DynamicBuffer.h"

namespace NArchive {
namespace NZip {
 
// static const char kEndOfString = '\0';
  
CInArchiveException::CInArchiveException(CCauseType aCause):
  Cause(aCause)
{}

bool CInArchive::Open(IInStream *aStream, const UINT64 *aSearchHeaderSizeLimit)
{
  m_Stream = aStream;
  if(m_Stream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition) != S_OK)
    return false;
  m_Position = m_StreamStartPosition;
  return FindAndReadMarker(aSearchHeaderSizeLimit);
}

void CInArchive::Close()
{
  m_Stream.Release();
}

//////////////////////////////////////
// Markers

inline bool TestMarkerCandidate(const void *aTestBytes, UINT32 &aValue)
{
  aValue = *((const UINT32 *)(aTestBytes));
  return (aValue == NSignature::kLocalFileHeader) ||
    (aValue == NSignature::kEndOfCentralDir);
}

bool CInArchive::FindAndReadMarker(const UINT64 *aSearchHeaderSizeLimit)
{
  m_ArchiveInfo.StartPosition = 0;
  m_Position = m_StreamStartPosition;
  if(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL) != S_OK)
    return false;

  BYTE aMarker[NSignature::kMarkerSize];
  UINT32 aProcessedSize; 
  ReadBytes(aMarker, NSignature::kMarkerSize, &aProcessedSize);
  if(aProcessedSize != NSignature::kMarkerSize)
    return false;
  if (TestMarkerCandidate(aMarker, m_Signature))
    return true;

  CByteDynamicBuffer aDynamicBuffer;
  static const UINT32 kSearchMarkerBufferSize = 0x10000;
  aDynamicBuffer.EnsureCapacity(kSearchMarkerBufferSize);
  BYTE *aBuffer = aDynamicBuffer;
  UINT32 aNumBytesPrev = NSignature::kMarkerSize - 1;
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
    if (aNumBytesInBuffer < NSignature::kMarkerSize)
      return false;
    UINT32 aNumTests = aNumBytesInBuffer - NSignature::kMarkerSize + 1;
    for(UINT32 aPos = 0; aPos < aNumTests; aPos++, aCurTestPos++)
    { 
      if (TestMarkerCandidate(aBuffer + aPos, m_Signature))
      {
        m_ArchiveInfo.StartPosition = aCurTestPos;
        m_Position = aCurTestPos + NSignature::kMarkerSize;
        if(m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL) != S_OK)
          return false;
        return true;
      }
    }
    aNumBytesPrev = aNumBytesInBuffer - aNumTests;
    memmove(aBuffer, aBuffer + aNumTests, aNumBytesPrev);
  }
}

//////////////////////////////////////
// Read Operations

HRESULT CInArchive::ReadBytes(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->Read(aData, aSize, &aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  IncreasePositionValue(aProcessedSizeReal);
  return aResult;
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


//////////////////////////////////
// Read headers

bool CInArchive::ReadSignature(UINT32 &aSignature)
{
  return ReadBytesAndTestSize(&aSignature, sizeof(aSignature));
}

AString CInArchive::ReadFileName(UINT32 aNameSize)
{
  if (aNameSize == 0)
    return AString();
  SafeReadBytes(m_NameBuffer.GetBuffer(aNameSize), aNameSize);
  m_NameBuffer.ReleaseBuffer(aNameSize);
  return m_NameBuffer;
}

void CInArchive::GetArchiveInfo(CInArchiveInfo &anArchiveInfo) const
{
  anArchiveInfo = m_ArchiveInfo;
}

void CInArchive::ThrowIncorrectArchiveException()
{
  throw CInArchiveException(CInArchiveException::kIncorrectArchive);
}

HRESULT CInArchive::ReadHeaders(CItemInfoExVector &anItems, CProgressVirt *aProgress)
{
  // m_Signature must be kLocalFileHeaderSignature or
  // kEndOfCentralDirSignature
  // m_Position points to next byte after signature

  anItems.Clear();

  if (aProgress != 0)
  {
    UINT64 aNumItems = anItems.Size();
    RETURN_IF_NOT_S_OK(aProgress->SetCompleted(&aNumItems));
  }
  // FSeek -=  sizeof(UINT32); // atention it's not real position 

  while(m_Signature == NSignature::kLocalFileHeader)
  {
    // FSeek points to next byte after signature
    NFileHeader::CLocalBlock aLocalHeader;
    CItemInfoEx anItemInfo;
    anItemInfo.LocalHeaderPosition = 
        UINT32(m_Position - m_StreamStartPosition - sizeof(UINT32)); // points to signature;
    SafeReadBytes(&aLocalHeader, sizeof(aLocalHeader));
    UINT32 aFileNameSize = aLocalHeader.NameSize;
    anItemInfo.Name = ReadFileName(aFileNameSize);
    /*
    if (!NItemName::IsNameLegal(anItemInfo.Name))
      ThrowIncorrectArchiveException();
    */
    anItemInfo.ExtractVersion.Version = aLocalHeader.ExtractVersion.Version;
    anItemInfo.ExtractVersion.HostOS = aLocalHeader.ExtractVersion.HostOS;
    anItemInfo.Flags = aLocalHeader.Flags & NFileHeader::NFlags::kUsedBitsMask; 
    anItemInfo.CompressionMethod = aLocalHeader.CompressionMethod;
    anItemInfo.Time =  aLocalHeader.Time;
    anItemInfo.FileCRC = aLocalHeader.FileCRC;
    anItemInfo.PackSize = aLocalHeader.PackSize;
    anItemInfo.UnPackSize = aLocalHeader.UnPackSize;

    anItemInfo.LocalExtraSize = aLocalHeader.ExtraSize;
    anItemInfo.FileHeaderWithNameSize = sizeof(UINT32) + sizeof(aLocalHeader) + aFileNameSize;

    IncreaseRealPosition(aLocalHeader.ExtraSize);

    if (anItemInfo.HasDescriptor())
    {
      const kBufferSize = (1 << 12);
      BYTE aBuffer[kBufferSize];
      UINT32 aNumBytesInBuffer = 0;
      UINT32 aPackedSize = 0;

      bool aDescriptorWasFound = false;
      while (true)
      {
        UINT32 aProcessedSize;
        RETURN_IF_NOT_S_OK(ReadBytes(aBuffer + aNumBytesInBuffer, 
            kBufferSize - aNumBytesInBuffer, &aProcessedSize));
        aNumBytesInBuffer += aProcessedSize;
        if (aNumBytesInBuffer < sizeof(NFileHeader::CDataDescriptor))
          ThrowIncorrectArchiveException();
        for (int i = 0; i <= aNumBytesInBuffer - 
            sizeof(NFileHeader::CDataDescriptor); i++)
        {
          const NFileHeader::CDataDescriptor &aDescriptor = 
            *(NFileHeader::CDataDescriptor *)(aBuffer + i);
          if (aDescriptor.Signature == NSignature::kDataDescriptor &&
            aDescriptor.PackSize == aPackedSize + i)
          {
            aDescriptorWasFound = true;
            anItemInfo.FileCRC = aDescriptor.FileCRC;
            anItemInfo.PackSize = aDescriptor.PackSize;
            anItemInfo.UnPackSize = aDescriptor.UnPackSize;
            IncreaseRealPosition(INT64(INT32(0 - (aNumBytesInBuffer - i - 
                sizeof(NFileHeader::CDataDescriptor)))));
            break;
          };
        }
        if (aDescriptorWasFound)
          break;
        aPackedSize += i;
        for (int j = 0; i < aNumBytesInBuffer; i++, j++)
          aBuffer[j] = aBuffer[i];    
        aNumBytesInBuffer = j;
      }
    }
    else
      IncreaseRealPosition(aLocalHeader.PackSize);

    anItems.Add(anItemInfo);
    if (aProgress != 0)
    {
      UINT64 aNumItems = anItems.Size();
      RETURN_IF_NOT_S_OK(aProgress->SetCompleted(&aNumItems));
    }
    if (!ReadSignature(m_Signature))
      break;
  }
  UINT32 aCentralDirectorySize = 0;
  UINT64 aCentralDirectoryStartOffset = m_Position - sizeof(UINT32);
  for(int i = 0; i < anItems.Size(); i++)
  {
    if (aProgress != 0)
    {
      UINT64 aNumItems = anItems.Size();
      RETURN_IF_NOT_S_OK(aProgress->SetCompleted(&aNumItems));
    }
    // if(m_Signature == NSignature::kEndOfCentralDir)
    //   break;
    if(m_Signature != NSignature::kCentralFileHeader)
      ThrowIncorrectArchiveException();
  
    NFileHeader::CBlock aHeader;
    SafeReadBytes(&aHeader, sizeof(aHeader));
    UINT32 aLocalHeaderOffset = aHeader.LocalHeaderOffset;
    aHeader.Flags &= NFileHeader::NFlags::kUsedBitsMask;
    
    int anIndex;
    int aLeft = 0, aRight = anItems.Size();
    while(true)
    {
      if (aLeft >= aRight)
        ThrowIncorrectArchiveException();
      anIndex = (aLeft + aRight) / 2;
      UINT32 aPosition = anItems[anIndex].LocalHeaderPosition;
      if (aLocalHeaderOffset == aPosition)
        break;
      if (aLocalHeaderOffset < aPosition)
        aRight = anIndex;
      else
        aLeft = anIndex + 1;
    }
    CItemInfoEx &anItemInfo = anItems[anIndex];
    anItemInfo.MadeByVersion.Version = aHeader.MadeByVersion.Version;
    anItemInfo.MadeByVersion.HostOS = aHeader.MadeByVersion.HostOS;

    CVersion aCentalHeaderExtractVersion;
    aCentalHeaderExtractVersion.Version = aHeader.ExtractVersion.Version;
    aCentalHeaderExtractVersion.HostOS = aHeader.ExtractVersion.HostOS;

    if (anItemInfo.ExtractVersion != aCentalHeaderExtractVersion ||
        anItemInfo.Flags != aHeader.Flags ||
        anItemInfo.CompressionMethod != aHeader.CompressionMethod ||
        anItemInfo.Time != aHeader.Time ||
        anItemInfo.FileCRC != aHeader.FileCRC ||
        anItemInfo.PackSize != aHeader.PackSize ||
        anItemInfo.UnPackSize != aHeader.UnPackSize)
      ThrowIncorrectArchiveException();

    AString aCentralName = ReadFileName(aHeader.NameSize);
    if (anItemInfo.Name.Length() != aCentralName.Length())
      ThrowIncorrectArchiveException(); // test it maybe better compare names
    anItemInfo.Name = aCentralName;

    anItemInfo.CentralExtraPosition = m_Position;
    anItemInfo.CentralExtraSize = aHeader.ExtraSize;
    anItemInfo.CommentSize = aHeader.CommentSize;
    if (aHeader.DiskNumberStart != 0)
      throw CInArchiveException(CInArchiveException::kMultiVolumeArchiveAreNotSupported);
    anItemInfo.InternalAttributes = aHeader.InternalAttributes;
    anItemInfo.ExternalAttributes = aHeader.ExternalAttributes;

    // May be these strings must be deleted
    if (anItemInfo.IsDirectory())
    {
      // if (anItemInfo.PackSize != 0 /*  || anItemInfo.UnPackSize != 0 */)
      //   ThrowIncorrectArchiveException();
      anItemInfo.UnPackSize = 0;
    }

    UINT32 aCurrentRecordSize = sizeof(UINT32) + sizeof(aHeader) + 
      aHeader.NameSize + aHeader.ExtraSize + aHeader.CommentSize;

    aCentralDirectorySize += aCurrentRecordSize;
    IncreaseRealPosition(aHeader.ExtraSize + aHeader.CommentSize);
    if (!ReadSignature(m_Signature))
      break;
  }
  if(m_Signature != NSignature::kEndOfCentralDir)
    ThrowIncorrectArchiveException();
  CEndOfCentralDirectoryRecord anEndOfCentralDirHeader;
  SafeReadBytes(&anEndOfCentralDirHeader, sizeof(anEndOfCentralDirHeader));
  if (anEndOfCentralDirHeader.ThisDiskNumber != 0 ||
      anEndOfCentralDirHeader.StartCentralDirectoryDiskNumber != 0)
     throw CInArchiveException(CInArchiveException::kMultiVolumeArchiveAreNotSupported);
  if (anEndOfCentralDirHeader.NumEntriesInCentaralDirectoryOnThisDisk != ((UINT16)anItems.Size()) ||
      anEndOfCentralDirHeader.NumEntriesInCentaralDirectory != ((UINT16)anItems.Size()) ||
      anEndOfCentralDirHeader.CentralDirectorySize != aCentralDirectorySize ||
      (anEndOfCentralDirHeader.CentralDirectoryStartOffset != aCentralDirectoryStartOffset &&
        (!anItems.IsEmpty())))
    ThrowIncorrectArchiveException();
  
  m_ArchiveInfo.CommentPosition = m_Position;
  m_ArchiveInfo.CommentSize = anEndOfCentralDirHeader.CommentSize;
  return S_OK;
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

IInStream* CInArchive::CreateStream()
{
  CComPtr<IInStream> aStream = m_Stream;
  return aStream.Detach();
}

bool CInArchive::SeekInArchive(UINT64 aPosition)
{
  UINT64 aNewPosition;
  if(m_Stream->Seek(aPosition, STREAM_SEEK_SET, &aNewPosition) != S_OK)
    return false;
  return (aNewPosition == aPosition);
}

}}
