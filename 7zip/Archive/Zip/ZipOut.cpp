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

void COutArchive::MoveBasePosition(UInt32 distanceToMove)
{
  m_BasePosition += distanceToMove; // test overflow
}

void COutArchive::PrepareWriteCompressedData(UInt16 fileNameLength)
{
  m_LocalFileHeaderSize = 4 + NFileHeader::kLocalBlockSize + fileNameLength;
}

void COutArchive::WriteBytes(const void *buffer, UInt32 size)
{
  UInt32 processedSize;
  if(m_Stream->Write(buffer, size, &processedSize) != S_OK)
    throw 0;
  if(processedSize != size)
    throw 0;
  m_BasePosition += size;
}

void COutArchive::WriteByte(Byte b)
{
  WriteBytes(&b, 1);
}

void COutArchive::WriteUInt16(UInt16 value)
{
  for (int i = 0; i < 2; i++)
  {
    WriteByte((Byte)value);
    value >>= 8;
  }
}

void COutArchive::WriteUInt32(UInt32 value)
{
  for (int i = 0; i < 4; i++)
  {
    WriteByte((Byte)value);
    value >>= 8;
  }
}

void COutArchive::WriteLocalHeader(const CItem &item)
{
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  WriteUInt32(NSignature::kLocalFileHeader);
  WriteByte(item.ExtractVersion.Version);
  WriteByte(item.ExtractVersion.HostOS);
  WriteUInt16(item.Flags);
  WriteUInt16(item.CompressionMethod);
  WriteUInt32(item.Time);
  WriteUInt32(item.FileCRC);
  WriteUInt32(item.PackSize);
  WriteUInt32(item.UnPackSize);
  WriteUInt16(item.Name.Length());
  WriteUInt16(item.LocalExtraSize); // test it;
  WriteBytes((const char *)item.Name, item.Name.Length());
  MoveBasePosition(item.LocalExtraSize + item.PackSize);
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
}

void COutArchive::WriteCentralHeader(const CItem &item)
{
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  WriteUInt32(NSignature::kCentralFileHeader);
  WriteByte(item.MadeByVersion.Version);
  WriteByte(item.MadeByVersion.HostOS);
  WriteByte(item.ExtractVersion.Version);
  WriteByte(item.ExtractVersion.HostOS);
  WriteUInt16(item.Flags);
  WriteUInt16(item.CompressionMethod);
  WriteUInt32(item.Time);
  WriteUInt32(item.FileCRC);
  WriteUInt32(item.PackSize);
  WriteUInt32(item.UnPackSize);
  WriteUInt16(item.Name.Length());
  WriteUInt16(item.CentralExtraSize); // test it;
  WriteUInt16(item.CommentSize);
  WriteUInt16(0); // DiskNumberStart;
  WriteUInt16(item.InternalAttributes);
  WriteUInt32(item.ExternalAttributes);
  WriteUInt32(item.LocalHeaderPosition);
  WriteBytes((const char *)item.Name, item.Name.Length());
}

void COutArchive::WriteEndOfCentralDir(const COutArchiveInfo &archiveInfo)
{
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  WriteUInt32(NSignature::kEndOfCentralDir);
  WriteUInt16(0); // ThisDiskNumber = 0;
  WriteUInt16(0); // StartCentralDirectoryDiskNumber;
  WriteUInt16(archiveInfo.NumEntriesInCentaralDirectory);
  WriteUInt16(archiveInfo.NumEntriesInCentaralDirectory);
  WriteUInt32(archiveInfo.CentralDirectorySize);
  WriteUInt32(archiveInfo.CentralDirectoryStartOffset);
  WriteUInt16(archiveInfo.CommentSize);
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
