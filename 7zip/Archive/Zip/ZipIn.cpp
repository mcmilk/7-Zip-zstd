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
  
bool CInArchive::Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit)
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

inline bool TestMarkerCandidate(const void *testBytes, UInt32 &value)
{
  value = *((const UInt32 *)(testBytes));
  return (value == NSignature::kLocalFileHeader) ||
    (value == NSignature::kEndOfCentralDir);
}

bool CInArchive::FindAndReadMarker(const UInt64 *searchHeaderSizeLimit)
{
  m_ArchiveInfo.StartPosition = 0;
  m_Position = m_StreamStartPosition;
  if(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL) != S_OK)
    return false;

  Byte marker[NSignature::kMarkerSize];
  UInt32 processedSize; 
  ReadBytes(marker, NSignature::kMarkerSize, &processedSize);
  if(processedSize != NSignature::kMarkerSize)
    return false;
  if (TestMarkerCandidate(marker, m_Signature))
    return true;

  CByteDynamicBuffer dynamicBuffer;
  static const UInt32 kSearchMarkerBufferSize = 0x10000;
  dynamicBuffer.EnsureCapacity(kSearchMarkerBufferSize);
  Byte *buffer = dynamicBuffer;
  UInt32 numBytesPrev = NSignature::kMarkerSize - 1;
  memmove(buffer, marker + 1, numBytesPrev);
  UInt64 curTestPos = m_StreamStartPosition + 1;
  while(true)
  {
    if (searchHeaderSizeLimit != NULL)
      if (curTestPos - m_StreamStartPosition > *searchHeaderSizeLimit)
        return false;
    UInt32 numReadBytes = kSearchMarkerBufferSize - numBytesPrev;
    ReadBytes(buffer + numBytesPrev, numReadBytes, &processedSize);
    UInt32 numBytesInBuffer = numBytesPrev + processedSize;
    if (numBytesInBuffer < NSignature::kMarkerSize)
      return false;
    UInt32 numTests = numBytesInBuffer - NSignature::kMarkerSize + 1;
    for(UInt32 pos = 0; pos < numTests; pos++, curTestPos++)
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

HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = m_Stream->Read(data, size, &realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  IncreasePositionValue(realProcessedSize);
  return result;
}

void CInArchive::IncreasePositionValue(UInt64 addValue)
{
  m_Position += addValue;
}

void CInArchive::IncreaseRealPosition(UInt64 addValue)
{
  if(m_Stream->Seek(addValue, STREAM_SEEK_CUR, &m_Position) != S_OK)
    throw CInArchiveException(CInArchiveException::kSeekStreamError);
}

bool CInArchive::ReadBytesAndTestSize(void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  if(ReadBytes(data, size, &realProcessedSize) != S_OK)
    throw CInArchiveException(CInArchiveException::kReadStreamError);
  return (realProcessedSize == size);
}

void CInArchive::SafeReadBytes(void *data, UInt32 size)
{
  if(!ReadBytesAndTestSize(data, size))
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
}

Byte CInArchive::ReadByte()
{
  Byte b;
  SafeReadBytes(&b, 1);
  return b;
}

UInt16 CInArchive::ReadUInt16()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = ReadByte();
    value |= (UInt16(b) << (8 * i));
  }
  return value;
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b = ReadByte();
    value |= (UInt32(b) << (8 * i));
  }
  return value;
}

//////////////////////////////////
// Read headers

bool CInArchive::ReadUInt32(UInt32 &value)
{
  value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b;
    if (!ReadBytesAndTestSize(&b, 1))
      return false;
    value |= (UInt32(b) << (8 * i));
  }
  return true;
}

AString CInArchive::ReadFileName(UInt32 nameSize)
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

static UInt32 GetUInt32(const Byte *data)
{
  return 
      ((UInt32)(Byte)data[0]) |
      (((UInt32)(Byte)data[1]) << 8) |
      (((UInt32)(Byte)data[2]) << 16) |
      (((UInt32)(Byte)data[3]) << 24);
}

HRESULT CInArchive::ReadHeaders(CObjectVector<CItemEx> &items, CProgressVirt *progress)
{
  // m_Signature must be kLocalFileHeaderSignature or
  // kEndOfCentralDirSignature
  // m_Position points to next byte after signature

  items.Clear();

  if (progress != 0)
  {
    UInt64 numItems = items.Size();
    RINOK(progress->SetCompleted(&numItems));
  }
  // FSeek -=  sizeof(UInt32); // atention it's not real position 

  while(m_Signature == NSignature::kLocalFileHeader)
  {
    // FSeek points to next byte after signature
    // NFileHeader::CLocalBlock localHeader;
    CItemEx itemInfo;
    itemInfo.LocalHeaderPosition = 
        UInt32(m_Position - m_StreamStartPosition - 4); // points to signature;
    
    // SafeReadBytes(&localHeader, sizeof(localHeader));

    itemInfo.ExtractVersion.Version = ReadByte();
    itemInfo.ExtractVersion.HostOS = ReadByte();
    itemInfo.Flags = ReadUInt16() & NFileHeader::NFlags::kUsedBitsMask; 
    itemInfo.CompressionMethod = ReadUInt16();
    itemInfo.Time =  ReadUInt32();
    itemInfo.FileCRC = ReadUInt32();
    itemInfo.PackSize = ReadUInt32();
    itemInfo.UnPackSize = ReadUInt32();
    UInt32 fileNameSize = ReadUInt16();
    itemInfo.LocalExtraSize = ReadUInt16();
    itemInfo.Name = ReadFileName(fileNameSize);
    /*
    if (!NItemName::IsNameLegal(itemInfo.Name))
      ThrowIncorrectArchiveException();
    */

    itemInfo.FileHeaderWithNameSize = 4 + 
        NFileHeader::kLocalBlockSize + fileNameSize;

    IncreaseRealPosition(itemInfo.LocalExtraSize);

    if (itemInfo.HasDescriptor())
    {
      const int kBufferSize = (1 << 12);
      Byte buffer[kBufferSize];

      UInt32 numBytesInBuffer = 0;
      UInt32 packedSize = 0;

      bool descriptorWasFound = false;
      while (true)
      {
        UInt32 processedSize;
        RINOK(ReadBytes(buffer + numBytesInBuffer, 
            kBufferSize - numBytesInBuffer, &processedSize));
        numBytesInBuffer += processedSize;
        if (numBytesInBuffer < NFileHeader::kDataDescriptorSize)
          ThrowIncorrectArchiveException();
        int i;
        for (i = 0; i <= numBytesInBuffer - NFileHeader::kDataDescriptorSize; i++)
        {
          UInt32 descriptorSignature = GetUInt32(buffer + i);
          UInt32 descriptorPackSize = GetUInt32(buffer + i + 8);
          if (descriptorSignature== NSignature::kDataDescriptor &&
            descriptorPackSize == packedSize + i)
          {
            descriptorWasFound = true;
            itemInfo.FileCRC = GetUInt32(buffer + i + 4);
            itemInfo.PackSize = descriptorPackSize;
            itemInfo.UnPackSize = GetUInt32(buffer + i + 12);
            IncreaseRealPosition(Int64(Int32(0 - (numBytesInBuffer - i - 
                NFileHeader::kDataDescriptorSize))));
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
      IncreaseRealPosition(itemInfo.PackSize);

    items.Add(itemInfo);
    if (progress != 0)
    {
      UInt64 numItems = items.Size();
      RINOK(progress->SetCompleted(&numItems));
    }
    if (!ReadUInt32(m_Signature))
      break;
  }
  UInt32 centralDirectorySize = 0;
  UInt64 centralDirectoryStartOffset = m_Position - 4;
  for(int i = 0; i < items.Size(); i++)
  {
    if (progress != 0)
    {
      UInt64 numItems = items.Size();
      RINOK(progress->SetCompleted(&numItems));
    }
    // if(m_Signature == NSignature::kEndOfCentralDir)
    //   break;
    if(m_Signature != NSignature::kCentralFileHeader)
      ThrowIncorrectArchiveException();
  
    // NFileHeader::CBlock header;
    // SafeReadBytes(&header, sizeof(header));

    Byte headerMadeByVersionVersion = ReadByte();
    Byte headerMadeByVersionHostOS = ReadByte();
    Byte centalHeaderExtractVersionVersion = ReadByte();
    Byte centalHeaderExtractVersionHostOS = ReadByte();
    UInt16 headerFlags = ReadUInt16() & NFileHeader::NFlags::kUsedBitsMask; 
    UInt16 headerCompressionMethod = ReadUInt16();
    UInt32 headerTime =  ReadUInt32();
    UInt32 headerFileCRC = ReadUInt32();
    UInt32 headerPackSize = ReadUInt32();
    UInt32 headerUnPackSize = ReadUInt32();
    UInt16 headerNameSize = ReadUInt16();
    UInt16 headerExtraSize = ReadUInt16();
    UInt16 headerCommentSize = ReadUInt16();
    UInt16 headerDiskNumberStart = ReadUInt16();
    UInt16 headerInternalAttributes = ReadUInt16();
    UInt16 headerExternalAttributes = ReadUInt32();
    UInt32 localHeaderOffset = ReadUInt32();
    
    // itemInfo.Name = ReadFileName(fileNameSize);

    
    int index;
    int left = 0, right = items.Size();
    while(true)
    {
      if (left >= right)
        ThrowIncorrectArchiveException();
      index = (left + right) / 2;
      UInt32 position = items[index].LocalHeaderPosition;
      if (localHeaderOffset == position)
        break;
      if (localHeaderOffset < position)
        right = index;
      else
        left = index + 1;
    }
    CItemEx &itemInfo = items[index];
    itemInfo.MadeByVersion.Version = headerMadeByVersionVersion;
    itemInfo.MadeByVersion.HostOS = headerMadeByVersionHostOS;

    if (
        // itemInfo.ExtractVersion != centalHeaderExtractVersion ||
        itemInfo.Flags != headerFlags ||
        itemInfo.CompressionMethod != headerCompressionMethod ||
        // itemInfo.Time != header.Time ||
        itemInfo.FileCRC != headerFileCRC ||
        itemInfo.PackSize != headerPackSize ||
        itemInfo.UnPackSize != headerUnPackSize)
      ThrowIncorrectArchiveException();

    AString centralName = ReadFileName(headerNameSize);
    if (itemInfo.Name.Length() != centralName.Length())
      ThrowIncorrectArchiveException(); // test it maybe better compare names
    itemInfo.Name = centralName;

    itemInfo.CentralExtraPosition = m_Position;
    itemInfo.CentralExtraSize = headerExtraSize;
    itemInfo.CommentSize = headerCommentSize;
    if (headerDiskNumberStart != 0)
      throw CInArchiveException(CInArchiveException::kMultiVolumeArchiveAreNotSupported);
    itemInfo.InternalAttributes = headerInternalAttributes;
    itemInfo.ExternalAttributes = headerExternalAttributes;

    // May be these strings must be deleted
    if (itemInfo.IsDirectory())
    {
      // if (itemInfo.PackSize != 0 /*  || itemInfo.UnPackSize != 0 */)
      //   ThrowIncorrectArchiveException();
      itemInfo.UnPackSize = 0;
    }

    UInt32 currentRecordSize = 4 + NFileHeader::kCentralBlockSize + 
        headerNameSize + headerExtraSize + headerCommentSize;

    centralDirectorySize += currentRecordSize;
    IncreaseRealPosition(headerExtraSize + headerCommentSize);
    if (!ReadUInt32(m_Signature))
      break;
  }
  if(m_Signature != NSignature::kEndOfCentralDir)
    ThrowIncorrectArchiveException();

  CEndOfCentralDirectoryRecord eocdh;
  eocdh.ThisDiskNumber = ReadUInt16();
  eocdh.StartCentralDirectoryDiskNumber = ReadUInt16();
  eocdh.NumEntriesInCentaralDirectoryOnThisDisk = ReadUInt16();
  eocdh.NumEntriesInCentaralDirectory = ReadUInt16();
  eocdh.CentralDirectorySize = ReadUInt32();
  eocdh.CentralDirectoryStartOffset = ReadUInt32();
  eocdh.CommentSize = ReadUInt16();

  if (eocdh.ThisDiskNumber != 0 ||
      eocdh.StartCentralDirectoryDiskNumber != 0)
     throw CInArchiveException(CInArchiveException::kMultiVolumeArchiveAreNotSupported);
  if (eocdh.NumEntriesInCentaralDirectoryOnThisDisk != ((UInt16)items.Size()) ||
      eocdh.NumEntriesInCentaralDirectory != ((UInt16)items.Size()) ||
      eocdh.CentralDirectorySize != centralDirectorySize ||
      (eocdh.CentralDirectoryStartOffset != centralDirectoryStartOffset &&
        (!items.IsEmpty())))
    ThrowIncorrectArchiveException();
  
  m_ArchiveInfo.CommentPosition = m_Position;
  m_ArchiveInfo.CommentSize = eocdh.CommentSize;
  return S_OK;
}

ISequentialInStream* CInArchive::CreateLimitedStream(UInt64 position, UInt64 size)
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

bool CInArchive::SeekInArchive(UInt64 position)
{
  UInt64 newPosition;
  if(m_Stream->Seek(position, STREAM_SEEK_SET, &newPosition) != S_OK)
    return false;
  return (newPosition == position);
}

}}

