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

static inline bool TestMarkerCandidate(const Byte *p, UInt32 &value)
{
  value = p[0] | (((UInt32)p[1]) << 8) | (((UInt32)p[2]) << 16) | (((UInt32)p[3]) << 24);
  return (value == NSignature::kLocalFileHeader) ||
    (value == NSignature::kEndOfCentralDir);
}

bool CInArchive::FindAndReadMarker(const UInt64 *searchHeaderSizeLimit)
{
  m_ArchiveInfo.Clear();
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
  m_Position += realProcessedSize;
  return result;
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

void CInArchive::ReadBuffer(CByteBuffer &buffer, UInt32 size)
{
  buffer.SetCapacity(size);
  if (size > 0)
    SafeReadBytes(buffer, size);
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
    value |= (((UInt16)ReadByte()) << (8 * i));
  return value;
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
    value |= (((UInt32)ReadByte()) << (8 * i));
  return value;
}

UInt64 CInArchive::ReadUInt64()
{
  UInt64 value = 0;
  for (int i = 0; i < 8; i++)
    value |= (((UInt64)ReadByte()) << (8 * i));
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


void CInArchive::ReadExtra(UInt32 extraSize, CExtraBlock &extraBlock, 
    UInt64 &unpackSize, UInt64 &packSize, UInt64 &localHeaderOffset, UInt32 &diskStartNumber)
{
  extraBlock.Clear();
  UInt32 remain = extraSize;
  while(remain >= 4)
  {
    CExtraSubBlock subBlock;
    subBlock.ID = ReadUInt16();
    UInt32 dataSize = ReadUInt16();
    remain -= 4;
    if (dataSize > remain) // it's bug
      dataSize = remain;
    if (subBlock.ID == 0x1)
    {
      if (unpackSize == 0xFFFFFFFF)
      {
        if (dataSize < 8)
          break;
        unpackSize = ReadUInt64();
        remain -= 8;
        dataSize -= 8;
      }
      if (packSize == 0xFFFFFFFF)
      {
        if (dataSize < 8)
          break;
        packSize = ReadUInt64();
        remain -= 8;
        dataSize -= 8;
      }
      if (localHeaderOffset == 0xFFFFFFFF)
      {
        if (dataSize < 8)
          break;
        localHeaderOffset = ReadUInt64();
        remain -= 8;
        dataSize -= 8;
      }
      if (diskStartNumber == 0xFFFF)
      {
        if (dataSize < 4)
          break;
        diskStartNumber = ReadUInt32();
        remain -= 4;
        dataSize -= 4;
      }
      for (UInt32 i = 0; i < dataSize; i++)
        ReadByte();
    }
    else
    {
      ReadBuffer(subBlock.Data, dataSize);
      extraBlock.SubBlocks.Add(subBlock);
    }
    remain -= dataSize;
  }
  IncreaseRealPosition(remain);
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

  while(m_Signature == NSignature::kLocalFileHeader)
  {
    // FSeek points to next byte after signature
    // NFileHeader::CLocalBlock localHeader;
    CItemEx item;
    item.LocalHeaderPosition = m_Position - m_StreamStartPosition - 4; // points to signature;
    
    // SafeReadBytes(&localHeader, sizeof(localHeader));

    item.ExtractVersion.Version = ReadByte();
    item.ExtractVersion.HostOS = ReadByte();
    item.Flags = ReadUInt16() & NFileHeader::NFlags::kUsedBitsMask; 
    item.CompressionMethod = ReadUInt16();
    item.Time =  ReadUInt32();
    item.FileCRC = ReadUInt32();
    item.PackSize = ReadUInt32();
    item.UnPackSize = ReadUInt32();
    UInt32 fileNameSize = ReadUInt16();
    item.LocalExtraSize = ReadUInt16();
    item.Name = ReadFileName(fileNameSize);
    /*
    if (!NItemName::IsNameLegal(item.Name))
      ThrowIncorrectArchiveException();
    */

    item.FileHeaderWithNameSize = 4 + 
        NFileHeader::kLocalBlockSize + fileNameSize;

    // IncreaseRealPosition(item.LocalExtraSize);
    if (item.LocalExtraSize > 0)
    {
      UInt64 localHeaderOffset = 0;
      UInt32 diskStartNumber = 0;
      CExtraBlock extraBlock;
      ReadExtra(item.LocalExtraSize, extraBlock, item.UnPackSize, item.PackSize, 
          localHeaderOffset, diskStartNumber);
    }

    if (item.HasDescriptor())
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
        UInt32 i;
        for (i = 0; i <= numBytesInBuffer - NFileHeader::kDataDescriptorSize; i++)
        {
          // descriptorSignature field is Info-ZIP's extension 
          // to Zip specification.
          UInt32 descriptorSignature = GetUInt32(buffer + i);

          // !!!! It must be fixed for Zip64 archives
          UInt32 descriptorPackSize = GetUInt32(buffer + i + 8);
          if (descriptorSignature== NSignature::kDataDescriptor &&
            descriptorPackSize == packedSize + i)
          {
            descriptorWasFound = true;
            item.FileCRC = GetUInt32(buffer + i + 4);
            item.PackSize = descriptorPackSize;
            item.UnPackSize = GetUInt32(buffer + i + 12);
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
      IncreaseRealPosition(item.PackSize);

    items.Add(item);
    if (progress != 0)
    {
      UInt64 numItems = items.Size();
      RINOK(progress->SetCompleted(&numItems));
    }
    if (!ReadUInt32(m_Signature))
      break;
  }
  UInt64 centralDirectorySize = 0;
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
    UInt64 headerPackSize = ReadUInt32();
    UInt64 headerUnPackSize = ReadUInt32();
    UInt16 headerNameSize = ReadUInt16();
    UInt16 headerExtraSize = ReadUInt16();
    UInt16 headerCommentSize = ReadUInt16();
    UInt32 headerDiskNumberStart = ReadUInt16();
    UInt16 headerInternalAttributes = ReadUInt16();
    UInt16 headerExternalAttributes = ReadUInt32();
    UInt64 localHeaderOffset = ReadUInt32();
    AString centralName = ReadFileName(headerNameSize);
    
    // item.Name = ReadFileName(fileNameSize);

    CExtraBlock centralExtra;
    if (headerExtraSize > 0)
    {
      ReadExtra(headerExtraSize, centralExtra, headerUnPackSize, headerPackSize, localHeaderOffset, headerDiskNumberStart);
    }
    
    int index;
    int left = 0, right = items.Size();
    while(true)
    {
      if (left >= right)
        ThrowIncorrectArchiveException();
      index = (left + right) / 2;
      UInt64 position = items[index].LocalHeaderPosition;
      if (localHeaderOffset == position)
        break;
      if (localHeaderOffset < position)
        right = index;
      else
        left = index + 1;
    }
    CItemEx &item = items[index];
    item.MadeByVersion.Version = headerMadeByVersionVersion;
    item.MadeByVersion.HostOS = headerMadeByVersionHostOS;
    item.CentralExtra = centralExtra;

    if (
        // item.ExtractVersion != centalHeaderExtractVersion ||
        item.Flags != headerFlags ||
        item.CompressionMethod != headerCompressionMethod ||
        // item.Time != header.Time ||
        item.FileCRC != headerFileCRC)
      ThrowIncorrectArchiveException();

    if (item.Name.Length() != centralName.Length())
      ThrowIncorrectArchiveException(); // test it maybe better compare names
    item.Name = centralName;

    // item.CentralExtraPosition = m_Position;
    // item.CentralExtraSize = headerExtraSize;
    // item.CommentSize = headerCommentSize;
    if (headerDiskNumberStart != 0)
      throw CInArchiveException(CInArchiveException::kMultiVolumeArchiveAreNotSupported);
    item.InternalAttributes = headerInternalAttributes;
    item.ExternalAttributes = headerExternalAttributes;

    // May be these strings must be deleted
    if (item.IsDirectory())
    {
      // if (item.PackSize != 0 /*  || item.UnPackSize != 0 */)
      //   ThrowIncorrectArchiveException();
      item.UnPackSize = 0;
    }

    UInt32 currentRecordSize = 4 + NFileHeader::kCentralBlockSize + 
        headerNameSize + headerExtraSize + headerCommentSize;

    centralDirectorySize += currentRecordSize;

    // IncreaseRealPosition(headerExtraSize);

    if (
        item.PackSize != headerPackSize ||
        item.UnPackSize != headerUnPackSize
        )
      ThrowIncorrectArchiveException();

    // IncreaseRealPosition(headerCommentSize);
    ReadBuffer(item.Comment, headerCommentSize);

    if (!ReadUInt32(m_Signature))
      break;
  }
  UInt32 thisDiskNumber = 0;
  UInt32 startCDDiskNumber = 0;
  UInt64 numEntriesInCDOnThisDisk = 0;
  UInt64 numEntriesInCD = 0;
  UInt64 cdSize = 0;
  UInt64 cdStartOffsetFromRecord = 0;
  bool isZip64 = false;
  UInt64 zip64EndOfCDStartOffset = m_Position - 4;
  if(m_Signature == NSignature::kZip64EndOfCentralDir)
  {
    isZip64 = true;
    UInt64 recordSize = ReadUInt64();
    UInt16 versionMade = ReadUInt16();
    UInt16 versionNeedExtract = ReadUInt16();
    thisDiskNumber = ReadUInt32();
    startCDDiskNumber = ReadUInt32();
    numEntriesInCDOnThisDisk = ReadUInt64();
    numEntriesInCD = ReadUInt64();
    cdSize = ReadUInt64();
    cdStartOffsetFromRecord = ReadUInt64();
    IncreaseRealPosition(recordSize - kZip64EndOfCentralDirRecordSize);
    if (!ReadUInt32(m_Signature))
      return S_FALSE;
    if (thisDiskNumber != 0 || startCDDiskNumber != 0)
      throw CInArchiveException(CInArchiveException::kMultiVolumeArchiveAreNotSupported);
    if (numEntriesInCDOnThisDisk != items.Size() ||
        numEntriesInCD != items.Size() ||
        cdSize != centralDirectorySize ||
        (cdStartOffsetFromRecord != centralDirectoryStartOffset &&
        (!items.IsEmpty())))
      ThrowIncorrectArchiveException();
  }
  if(m_Signature == NSignature::kZip64EndOfCentralDirLocator)
  {
    UInt32 startEndCDDiskNumber = ReadUInt32();
    UInt64 endCDStartOffset = ReadUInt64();
    UInt32 numberOfDisks = ReadUInt32();
    if (zip64EndOfCDStartOffset != endCDStartOffset)
      ThrowIncorrectArchiveException();
    if (!ReadUInt32(m_Signature))
      return S_FALSE;
  }
  if(m_Signature != NSignature::kEndOfCentralDir)
    ThrowIncorrectArchiveException();

  UInt16 thisDiskNumber16 = ReadUInt16();
  if (!isZip64 || thisDiskNumber16)
    thisDiskNumber = thisDiskNumber16;

  UInt16 startCDDiskNumber16 = ReadUInt16();
  if (!isZip64 || startCDDiskNumber16 != 0xFFFF)
    startCDDiskNumber = startCDDiskNumber16;

  UInt16 numEntriesInCDOnThisDisk16 = ReadUInt16();
  if (!isZip64 || numEntriesInCDOnThisDisk16 != 0xFFFF)
    numEntriesInCDOnThisDisk = numEntriesInCDOnThisDisk16;

  UInt16 numEntriesInCD16 = ReadUInt16();
  if (!isZip64 || numEntriesInCD16 != 0xFFFF)
    numEntriesInCD = numEntriesInCD16;

  UInt32 cdSize32 = ReadUInt32();
  if (!isZip64 || cdSize32 != 0xFFFFFFFF)
    cdSize = cdSize32;

  UInt32 cdStartOffsetFromRecord32 = ReadUInt32();
  if (!isZip64 || cdStartOffsetFromRecord32 != 0xFFFFFFFF)
    cdStartOffsetFromRecord = cdStartOffsetFromRecord32;

  UInt16 commentSize = ReadUInt16();
  ReadBuffer(m_ArchiveInfo.Comment, commentSize);
  
  if (thisDiskNumber != 0 || startCDDiskNumber != 0)
    throw CInArchiveException(CInArchiveException::kMultiVolumeArchiveAreNotSupported);
  if ((UInt16)numEntriesInCDOnThisDisk != ((UInt16)items.Size()) ||
      (UInt16)numEntriesInCD != ((UInt16)items.Size()) ||
      (UInt32)cdSize != (UInt32)centralDirectorySize ||
      ((UInt32)(cdStartOffsetFromRecord) != (UInt32)centralDirectoryStartOffset &&
        (!items.IsEmpty())))
    ThrowIncorrectArchiveException();
  
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

