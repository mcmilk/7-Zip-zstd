// Archive/ZipIn.cpp

#include "StdAfx.h"

#include "ZipIn.h"
#include "Windows/Defs.h"
#include "Common/StringConvert.h"
#include "Common/DynamicBuffer.h"
#include "../../Common/LimitedStreams.h"

namespace NArchive {
namespace NZip {
 
// static const char kEndOfString = '\0';
  
bool CInArchive::Open(IInStream *inStream, const UINT64 *searchHeaderSizeLimit)
{
  m_Stream = inStream;
  if(m_Stream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition) != S_OK)
    return false;
  m_Position = m_StreamStartPosition;
  return FindAndReadMarker(searchHeaderSizeLimit);
}

void CInArchive::Close()
{
  m_Stream.Release();
}

//////////////////////////////////////
// Markers

inline bool TestMarkerCandidate(const void *testBytes, UINT32 &value)
{
  value = *((const UINT32 *)(testBytes));
  return (value == NSignature::kLocalFileHeader) ||
    (value == NSignature::kEndOfCentralDir);
}

bool CInArchive::FindAndReadMarker(const UINT64 *searchHeaderSizeLimit)
{
  m_ArchiveInfo.StartPosition = 0;
  m_Position = m_StreamStartPosition;
  if(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL) != S_OK)
    return false;

  BYTE marker[NSignature::kMarkerSize];
  UINT32 processedSize; 
  ReadBytes(marker, NSignature::kMarkerSize, &processedSize);
  if(processedSize != NSignature::kMarkerSize)
    return false;
  if (TestMarkerCandidate(marker, m_Signature))
    return true;

  CByteDynamicBuffer dynamicBuffer;
  static const UINT32 kSearchMarkerBufferSize = 0x10000;
  dynamicBuffer.EnsureCapacity(kSearchMarkerBufferSize);
  BYTE *buffer = dynamicBuffer;
  UINT32 numBytesPrev = NSignature::kMarkerSize - 1;
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
    if (numBytesInBuffer < NSignature::kMarkerSize)
      return false;
    UINT32 numTests = numBytesInBuffer - NSignature::kMarkerSize + 1;
    for(UINT32 pos = 0; pos < numTests; pos++, curTestPos++)
    { 
      if (TestMarkerCandidate(buffer + pos, m_Signature))
      {
        m_ArchiveInfo.StartPosition = curTestPos;
        m_Position = curTestPos + NSignature::kMarkerSize;
        if(m_Stream->Seek(m_Position, STREAM_SEEK_SET, NULL) != S_OK)
          return false;
        return true;
      }
    }
    numBytesPrev = numBytesInBuffer - numTests;
    memmove(buffer, buffer + numTests, numBytesPrev);
  }
}

//////////////////////////////////////
// Read Operations

HRESULT CInArchive::ReadBytes(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = m_Stream->Read(data, size, &realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  IncreasePositionValue(realProcessedSize);
  return result;
}

void CInArchive::IncreasePositionValue(UINT64 addValue)
{
  m_Position += addValue;
}

void CInArchive::IncreaseRealPosition(UINT64 addValue)
{
  if(m_Stream->Seek(addValue, STREAM_SEEK_CUR, &m_Position) != S_OK)
    throw CInArchiveException(CInArchiveException::kSeekStreamError);
}

bool CInArchive::ReadBytesAndTestSize(void *data, UINT32 size)
{
  UINT32 realProcessedSize;
  if(ReadBytes(data, size, &realProcessedSize) != S_OK)
    throw CInArchiveException(CInArchiveException::kReadStreamError);
  return (realProcessedSize == size);
}

void CInArchive::SafeReadBytes(void *data, UINT32 size)
{
  if(!ReadBytesAndTestSize(data, size))
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
}


//////////////////////////////////
// Read headers

bool CInArchive::ReadSignature(UINT32 &signature)
{
  return ReadBytesAndTestSize(&signature, sizeof(signature));
}

AString CInArchive::ReadFileName(UINT32 nameSize)
{
  if (nameSize == 0)
    return AString();
  SafeReadBytes(m_NameBuffer.GetBuffer(nameSize), nameSize);
  m_NameBuffer.ReleaseBuffer(nameSize);
  return m_NameBuffer;
}

void CInArchive::GetArchiveInfo(CInArchiveInfo &archiveInfo) const
{
  archiveInfo = m_ArchiveInfo;
}

void CInArchive::ThrowIncorrectArchiveException()
{
  throw CInArchiveException(CInArchiveException::kIncorrectArchive);
}

HRESULT CInArchive::ReadHeaders(CObjectVector<CItemEx> &items, CProgressVirt *progress)
{
  // m_Signature must be kLocalFileHeaderSignature or
  // kEndOfCentralDirSignature
  // m_Position points to next byte after signature

  items.Clear();

  if (progress != 0)
  {
    UINT64 numItems = items.Size();
    RINOK(progress->SetCompleted(&numItems));
  }
  // FSeek -=  sizeof(UINT32); // atention it's not real position 

  while(m_Signature == NSignature::kLocalFileHeader)
  {
    // FSeek points to next byte after signature
    NFileHeader::CLocalBlock localHeader;
    CItemEx itemInfo;
    itemInfo.LocalHeaderPosition = 
        UINT32(m_Position - m_StreamStartPosition - sizeof(UINT32)); // points to signature;
    SafeReadBytes(&localHeader, sizeof(localHeader));
    UINT32 fileNameSize = localHeader.NameSize;
    itemInfo.Name = ReadFileName(fileNameSize);
    /*
    if (!NItemName::IsNameLegal(itemInfo.Name))
      ThrowIncorrectArchiveException();
    */
    itemInfo.ExtractVersion.Version = localHeader.ExtractVersion.Version;
    itemInfo.ExtractVersion.HostOS = localHeader.ExtractVersion.HostOS;
    itemInfo.Flags = localHeader.Flags & NFileHeader::NFlags::kUsedBitsMask; 
    itemInfo.CompressionMethod = localHeader.CompressionMethod;
    itemInfo.Time =  localHeader.Time;
    itemInfo.FileCRC = localHeader.FileCRC;
    itemInfo.PackSize = localHeader.PackSize;
    itemInfo.UnPackSize = localHeader.UnPackSize;

    itemInfo.LocalExtraSize = localHeader.ExtraSize;
    itemInfo.FileHeaderWithNameSize = sizeof(UINT32) + sizeof(localHeader) + fileNameSize;

    IncreaseRealPosition(localHeader.ExtraSize);

    if (itemInfo.HasDescriptor())
    {
      const int kBufferSize = (1 << 12);
      BYTE buffer[kBufferSize];
      UINT32 numBytesInBuffer = 0;
      UINT32 packedSize = 0;

      bool descriptorWasFound = false;
      while (true)
      {
        UINT32 processedSize;
        RINOK(ReadBytes(buffer + numBytesInBuffer, 
            kBufferSize - numBytesInBuffer, &processedSize));
        numBytesInBuffer += processedSize;
        if (numBytesInBuffer < sizeof(NFileHeader::CDataDescriptor))
          ThrowIncorrectArchiveException();
        int i;
        for (i = 0; i <= numBytesInBuffer - 
            sizeof(NFileHeader::CDataDescriptor); i++)
        {
          const NFileHeader::CDataDescriptor &descriptor = 
            *(NFileHeader::CDataDescriptor *)(buffer + i);
          if (descriptor.Signature == NSignature::kDataDescriptor &&
            descriptor.PackSize == packedSize + i)
          {
            descriptorWasFound = true;
            itemInfo.FileCRC = descriptor.FileCRC;
            itemInfo.PackSize = descriptor.PackSize;
            itemInfo.UnPackSize = descriptor.UnPackSize;
            IncreaseRealPosition(INT64(INT32(0 - (numBytesInBuffer - i - 
                sizeof(NFileHeader::CDataDescriptor)))));
            break;
          };
        }
        if (descriptorWasFound)
          break;
        packedSize += i;
        int j;
        for (j = 0; i < numBytesInBuffer; i++, j++)
          buffer[j] = buffer[i];    
        numBytesInBuffer = j;
      }
    }
    else
      IncreaseRealPosition(localHeader.PackSize);

    items.Add(itemInfo);
    if (progress != 0)
    {
      UINT64 numItems = items.Size();
      RINOK(progress->SetCompleted(&numItems));
    }
    if (!ReadSignature(m_Signature))
      break;
  }
  UINT32 centralDirectorySize = 0;
  UINT64 centralDirectoryStartOffset = m_Position - sizeof(UINT32);
  for(int i = 0; i < items.Size(); i++)
  {
    if (progress != 0)
    {
      UINT64 numItems = items.Size();
      RINOK(progress->SetCompleted(&numItems));
    }
    // if(m_Signature == NSignature::kEndOfCentralDir)
    //   break;
    if(m_Signature != NSignature::kCentralFileHeader)
      ThrowIncorrectArchiveException();
  
    NFileHeader::CBlock header;
    SafeReadBytes(&header, sizeof(header));
    UINT32 localHeaderOffset = header.LocalHeaderOffset;
    header.Flags &= NFileHeader::NFlags::kUsedBitsMask;
    
    int index;
    int left = 0, right = items.Size();
    while(true)
    {
      if (left >= right)
        ThrowIncorrectArchiveException();
      index = (left + right) / 2;
      UINT32 position = items[index].LocalHeaderPosition;
      if (localHeaderOffset == position)
        break;
      if (localHeaderOffset < position)
        right = index;
      else
        left = index + 1;
    }
    CItemEx &itemInfo = items[index];
    itemInfo.MadeByVersion.Version = header.MadeByVersion.Version;
    itemInfo.MadeByVersion.HostOS = header.MadeByVersion.HostOS;

    CVersion centalHeaderExtractVersion;
    centalHeaderExtractVersion.Version = header.ExtractVersion.Version;
    centalHeaderExtractVersion.HostOS = header.ExtractVersion.HostOS;

    if (
        // itemInfo.ExtractVersion != centalHeaderExtractVersion ||
        itemInfo.Flags != header.Flags ||
        itemInfo.CompressionMethod != header.CompressionMethod ||
        // itemInfo.Time != header.Time ||
        itemInfo.FileCRC != header.FileCRC ||
        itemInfo.PackSize != header.PackSize ||
        itemInfo.UnPackSize != header.UnPackSize)
      ThrowIncorrectArchiveException();

    AString centralName = ReadFileName(header.NameSize);
    if (itemInfo.Name.Length() != centralName.Length())
      ThrowIncorrectArchiveException(); // test it maybe better compare names
    itemInfo.Name = centralName;

    itemInfo.CentralExtraPosition = m_Position;
    itemInfo.CentralExtraSize = header.ExtraSize;
    itemInfo.CommentSize = header.CommentSize;
    if (header.DiskNumberStart != 0)
      throw CInArchiveException(CInArchiveException::kMultiVolumeArchiveAreNotSupported);
    itemInfo.InternalAttributes = header.InternalAttributes;
    itemInfo.ExternalAttributes = header.ExternalAttributes;

    // May be these strings must be deleted
    if (itemInfo.IsDirectory())
    {
      // if (itemInfo.PackSize != 0 /*  || itemInfo.UnPackSize != 0 */)
      //   ThrowIncorrectArchiveException();
      itemInfo.UnPackSize = 0;
    }

    UINT32 currentRecordSize = sizeof(UINT32) + sizeof(header) + 
      header.NameSize + header.ExtraSize + header.CommentSize;

    centralDirectorySize += currentRecordSize;
    IncreaseRealPosition(header.ExtraSize + header.CommentSize);
    if (!ReadSignature(m_Signature))
      break;
  }
  if(m_Signature != NSignature::kEndOfCentralDir)
    ThrowIncorrectArchiveException();
  CEndOfCentralDirectoryRecord endOfCentralDirHeader;
  SafeReadBytes(&endOfCentralDirHeader, sizeof(endOfCentralDirHeader));
  if (endOfCentralDirHeader.ThisDiskNumber != 0 ||
      endOfCentralDirHeader.StartCentralDirectoryDiskNumber != 0)
     throw CInArchiveException(CInArchiveException::kMultiVolumeArchiveAreNotSupported);
  if (endOfCentralDirHeader.NumEntriesInCentaralDirectoryOnThisDisk != ((UINT16)items.Size()) ||
      endOfCentralDirHeader.NumEntriesInCentaralDirectory != ((UINT16)items.Size()) ||
      endOfCentralDirHeader.CentralDirectorySize != centralDirectorySize ||
      (endOfCentralDirHeader.CentralDirectoryStartOffset != centralDirectoryStartOffset &&
        (!items.IsEmpty())))
    ThrowIncorrectArchiveException();
  
  m_ArchiveInfo.CommentPosition = m_Position;
  m_ArchiveInfo.CommentSize = endOfCentralDirHeader.CommentSize;
  return S_OK;
}

ISequentialInStream* CInArchive::CreateLimitedStream(UINT64 position, UINT64 size)
{
  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  SeekInArchive(position);
  streamSpec->Init(m_Stream, size);
  return inStream.Detach();
}

IInStream* CInArchive::CreateStream()
{
  CMyComPtr<IInStream> inStream = m_Stream;
  return inStream.Detach();
}

bool CInArchive::SeekInArchive(UINT64 position)
{
  UINT64 newPosition;
  if(m_Stream->Seek(position, STREAM_SEEK_SET, &newPosition) != S_OK)
    return false;
  return (newPosition == position);
}

}}
