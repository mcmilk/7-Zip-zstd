// ZipOutEngine.cpp

#include "StdAfx.h"

#include "OutEngine.h"
#include "Common/StringConvert.h"
#include "Common/CRC.h"
#include "Interface/OffsetStream.h"

namespace NArchive {
namespace NZip {

void COutArchive::Create(IOutStream *aStream)
{
  m_Stream = aStream;
  m_BasePosition = 0;
}

void COutArchive::MoveBasePosition(UINT32 aDistanceToMove)
{
  m_BasePosition += aDistanceToMove; // test overflow
}

void COutArchive::PrepareWriteCompressedData(UINT16 aFileNameLength)
{
  m_LocalFileHeaderSize = sizeof(NFileHeader::CLocalBlockFull) + aFileNameLength;
}

void COutArchive::WriteBytes(const void *aBuffer, UINT32 aSize)
{
  UINT32 aProcessedSize;
  if(m_Stream->Write(aBuffer, aSize, &aProcessedSize) != S_OK)
    throw 0;
  if(aProcessedSize != aSize)
    throw 0;
  m_BasePosition += aSize;
}

void COutArchive::WriteLocalHeader(const CItemInfo &anItemInfo)
{
  NFileHeader::CLocalBlockFull aFileHeader;
  aFileHeader.Signature = NSignature::kLocalFileHeader;
  NFileHeader::CLocalBlock &aHeader = aFileHeader.Header;

  aHeader.ExtractVersion.HostOS = anItemInfo.ExtractVersion.HostOS;
  aHeader.ExtractVersion.Version = anItemInfo.ExtractVersion.Version;
  aHeader.Flags = anItemInfo.Flags;
  aHeader.CompressionMethod = anItemInfo.CompressionMethod;
  aHeader.Time = anItemInfo.Time;
  aHeader.FileCRC = anItemInfo.FileCRC;
  aHeader.PackSize = anItemInfo.PackSize;
  aHeader.UnPackSize = anItemInfo.UnPackSize;

  aHeader.NameSize = anItemInfo.Name.Length();
  aHeader.ExtraSize = anItemInfo.LocalExtraSize; // test it;

  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  WriteBytes(&aFileHeader, sizeof(aFileHeader));
  WriteBytes((const char *)anItemInfo.Name, aHeader.NameSize);
  MoveBasePosition(aHeader.ExtraSize + aHeader.PackSize);
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
}

void COutArchive::WriteCentralHeader(const CItemInfo &anItemInfo)
{
  NFileHeader::CBlockFull aFileHeader;
  aFileHeader.Signature = NSignature::kCentralFileHeader;
  NFileHeader::CBlock &aHeader = aFileHeader.Header;

  aHeader.MadeByVersion.HostOS = anItemInfo.MadeByVersion.HostOS;
   aHeader.MadeByVersion.Version = anItemInfo.MadeByVersion.Version;
  aHeader.ExtractVersion.HostOS = anItemInfo.ExtractVersion.HostOS;
  aHeader.ExtractVersion.Version = anItemInfo.ExtractVersion.Version;
  aHeader.Flags = anItemInfo.Flags;
  aHeader.CompressionMethod = anItemInfo.CompressionMethod;
  aHeader.Time = anItemInfo.Time;
  aHeader.FileCRC = anItemInfo.FileCRC;
  aHeader.PackSize = anItemInfo.PackSize;
  aHeader.UnPackSize = anItemInfo.UnPackSize;

  aHeader.NameSize = anItemInfo.Name.Length();

  aHeader.ExtraSize = anItemInfo.CentralExtraSize; // test it;
  aHeader.CommentSize = anItemInfo.CommentSize;
  aHeader.DiskNumberStart = 0;
  aHeader.InternalAttributes = anItemInfo.InternalAttributes;
  aHeader.ExternalAttributes = anItemInfo.ExternalAttributes;
  aHeader.LocalHeaderOffset = anItemInfo.LocalHeaderPosition;

  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  WriteBytes(&aFileHeader, sizeof(aFileHeader));
  WriteBytes((const char *)anItemInfo.Name, aHeader.NameSize);
}

void COutArchive::WriteEndOfCentralDir(const COutArchiveInfo &anArchiveInfo)
{
  CEndOfCentralDirectoryRecordFull aFileHeader;
  aFileHeader.Signature = NSignature::kEndOfCentralDir;
  CEndOfCentralDirectoryRecord &aHeader = aFileHeader.Header;
  
  aHeader.ThisDiskNumber = 0;
  aHeader.StartCentralDirectoryDiskNumber = 0;
  
  aHeader.NumEntriesInCentaralDirectoryOnThisDisk = anArchiveInfo.NumEntriesInCentaralDirectory;
  aHeader.NumEntriesInCentaralDirectory = anArchiveInfo.NumEntriesInCentaralDirectory;
  
  aHeader.CentralDirectorySize = anArchiveInfo.CentralDirectorySize;
  aHeader.CentralDirectoryStartOffset = anArchiveInfo.CentralDirectoryStartOffset;
  aHeader.CommentSize = anArchiveInfo.CommentSize;

  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  WriteBytes(&aFileHeader, sizeof(aFileHeader));
}

void COutArchive::CreateStreamForCompressing(IOutStream **anOutStream)
{
  CComObjectNoLock<COffsetOutStream> *aStreamSpec = new 
      CComObjectNoLock<COffsetOutStream>;
  CComPtr<IOutStream> aStream(aStreamSpec);
  aStreamSpec->Init(m_Stream, m_BasePosition + m_LocalFileHeaderSize);
  *anOutStream = aStream.Detach();
}

void COutArchive::SeekToPackedDataPosition()
{
  m_Stream->Seek(m_BasePosition + m_LocalFileHeaderSize, STREAM_SEEK_SET, NULL);
}


void COutArchive::CreateStreamForCopying(ISequentialOutStream **anOutStream)
{
  CComPtr<ISequentialOutStream> aStream = m_Stream;
  *anOutStream = aStream.Detach();
}

}}
