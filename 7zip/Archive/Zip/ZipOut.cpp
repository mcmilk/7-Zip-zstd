// ZipOut.cpp

#include "StdAfx.h"

#include "ZipOut.h"
#include "Common/StringConvert.h"
#include "Common/CRC.h"
#include "../../Common/OffsetStream.h"

namespace NArchive {
namespace NZip {

void COutArchive::Create(IOutStream *outStream)
{
  m_Stream = outStream;
  m_BasePosition = 0;
}

void COutArchive::MoveBasePosition(UINT32 distanceToMove)
{
  m_BasePosition += distanceToMove; // test overflow
}

void COutArchive::PrepareWriteCompressedData(UINT16 fileNameLength)
{
  m_LocalFileHeaderSize = sizeof(NFileHeader::CLocalBlockFull) + fileNameLength;
}

void COutArchive::WriteBytes(const void *buffer, UINT32 size)
{
  UINT32 processedSize;
  if(m_Stream->Write(buffer, size, &processedSize) != S_OK)
    throw 0;
  if(processedSize != size)
    throw 0;
  m_BasePosition += size;
}

void COutArchive::WriteLocalHeader(const CItem &item)
{
  NFileHeader::CLocalBlockFull fileHeader;
  fileHeader.Signature = NSignature::kLocalFileHeader;
  NFileHeader::CLocalBlock &header = fileHeader.Header;

  header.ExtractVersion.HostOS = item.ExtractVersion.HostOS;
  header.ExtractVersion.Version = item.ExtractVersion.Version;
  header.Flags = item.Flags;
  header.CompressionMethod = item.CompressionMethod;
  header.Time = item.Time;
  header.FileCRC = item.FileCRC;
  header.PackSize = item.PackSize;
  header.UnPackSize = item.UnPackSize;

  header.NameSize = item.Name.Length();
  header.ExtraSize = item.LocalExtraSize; // test it;

  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  WriteBytes(&fileHeader, sizeof(fileHeader));
  WriteBytes((const char *)item.Name, header.NameSize);
  MoveBasePosition(header.ExtraSize + header.PackSize);
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
}

void COutArchive::WriteCentralHeader(const CItem &item)
{
  NFileHeader::CBlockFull fileHeader;
  fileHeader.Signature = NSignature::kCentralFileHeader;
  NFileHeader::CBlock &header = fileHeader.Header;

  header.MadeByVersion.HostOS = item.MadeByVersion.HostOS;
  header.MadeByVersion.Version = item.MadeByVersion.Version;
  header.ExtractVersion.HostOS = item.ExtractVersion.HostOS;
  header.ExtractVersion.Version = item.ExtractVersion.Version;
  header.Flags = item.Flags;
  header.CompressionMethod = item.CompressionMethod;
  header.Time = item.Time;
  header.FileCRC = item.FileCRC;
  header.PackSize = item.PackSize;
  header.UnPackSize = item.UnPackSize;

  header.NameSize = item.Name.Length();

  header.ExtraSize = item.CentralExtraSize; // test it;
  header.CommentSize = item.CommentSize;
  header.DiskNumberStart = 0;
  header.InternalAttributes = item.InternalAttributes;
  header.ExternalAttributes = item.ExternalAttributes;
  header.LocalHeaderOffset = item.LocalHeaderPosition;

  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  WriteBytes(&fileHeader, sizeof(fileHeader));
  WriteBytes((const char *)item.Name, header.NameSize);
}

void COutArchive::WriteEndOfCentralDir(const COutArchiveInfo &archiveInfo)
{
  CEndOfCentralDirectoryRecordFull fileHeader;
  fileHeader.Signature = NSignature::kEndOfCentralDir;
  CEndOfCentralDirectoryRecord &header = fileHeader.Header;
  
  header.ThisDiskNumber = 0;
  header.StartCentralDirectoryDiskNumber = 0;
  
  header.NumEntriesInCentaralDirectoryOnThisDisk = archiveInfo.NumEntriesInCentaralDirectory;
  header.NumEntriesInCentaralDirectory = archiveInfo.NumEntriesInCentaralDirectory;
  
  header.CentralDirectorySize = archiveInfo.CentralDirectorySize;
  header.CentralDirectoryStartOffset = archiveInfo.CentralDirectoryStartOffset;
  header.CommentSize = archiveInfo.CommentSize;

  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  WriteBytes(&fileHeader, sizeof(fileHeader));
}

void COutArchive::CreateStreamForCompressing(IOutStream **outStream)
{
  COffsetOutStream *streamSpec = new COffsetOutStream;
  CMyComPtr<IOutStream> tempStream(streamSpec);
  streamSpec->Init(m_Stream, m_BasePosition + m_LocalFileHeaderSize);
  *outStream = tempStream.Detach();
}

void COutArchive::SeekToPackedDataPosition()
{
  m_Stream->Seek(m_BasePosition + m_LocalFileHeaderSize, STREAM_SEEK_SET, NULL);
}

void COutArchive::CreateStreamForCopying(ISequentialOutStream **outStream)
{
  CMyComPtr<ISequentialOutStream> tempStream(m_Stream);
  *outStream = tempStream.Detach();
}

}}
