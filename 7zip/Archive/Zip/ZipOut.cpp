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

void COutArchive::MoveBasePosition(UInt64 distanceToMove)
{
  m_BasePosition += distanceToMove; // test overflow
}

void COutArchive::PrepareWriteCompressedDataZip64(UInt16 fileNameLength, bool isZip64)
{
  m_ExtraSize = isZip64 ? (4 + 8 + 8) : 0;
  m_LocalFileHeaderSize = 4 + NFileHeader::kLocalBlockSize + fileNameLength + m_ExtraSize;
}

void COutArchive::PrepareWriteCompressedData(UInt16 fileNameLength, UInt64 unPackSize)
{
  PrepareWriteCompressedDataZip64(fileNameLength, unPackSize >= 0xF0000000);
}

void COutArchive::PrepareWriteCompressedData2(UInt16 fileNameLength, UInt64 unPackSize, UInt64 packSize)
{
  bool isUnPack64 = unPackSize >= 0xFFFFFFFF;
  bool isPack64 = packSize >= 0xFFFFFFFF;
  bool isZip64 = isPack64 || isUnPack64;
  PrepareWriteCompressedDataZip64(fileNameLength, isZip64);
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

void COutArchive::WriteUInt64(UInt64 value)
{
  for (int i = 0; i < 8; i++)
  {
    WriteByte((Byte)value);
    value >>= 8;
  }
}

HRESULT COutArchive::WriteLocalHeader(const CItem &item)
{
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  
  bool isPack64 = item.PackSize >= 0xFFFFFFFF;
  bool isUnPack64 = item.UnPackSize >= 0xFFFFFFFF;
  bool isZip64  = isPack64 || isUnPack64;
  isPack64 = isZip64;
  isUnPack64 = isZip64;
  
  WriteUInt32(NSignature::kLocalFileHeader);
  WriteByte(item.ExtractVersion.Version);
  WriteByte(item.ExtractVersion.HostOS);
  WriteUInt16(item.Flags);
  WriteUInt16(item.CompressionMethod);
  WriteUInt32(item.Time);
  WriteUInt32(item.FileCRC);
  WriteUInt32(isPack64 ? 0xFFFFFFFF: (UInt32)item.PackSize);
  WriteUInt32(isUnPack64 ? 0xFFFFFFFF: (UInt32)item.UnPackSize);
  WriteUInt16(item.Name.Length());
  UInt16 localExtraSize = 
      isZip64 ? 
      (4 + 
        (isUnPack64 ? 8: 0) + 
        (isPack64 ? 8: 0)
      ):0;
  if (localExtraSize > m_ExtraSize)
    return E_FAIL;
  WriteUInt16(m_ExtraSize); // test it;
  WriteBytes((const char *)item.Name, item.Name.Length());
  if (m_ExtraSize > 0)
  {
    UInt16 remain = m_ExtraSize - 4; 
    WriteUInt16(0x1); // Zip64 Tag;
    WriteUInt16(remain);
    if (isZip64)
    {
      if(isUnPack64)
      {
        WriteUInt64(item.UnPackSize);
        remain -= 8;
      }
      if(isPack64)
      {
        WriteUInt64(item.PackSize);
        remain -= 8;
      }
      for (int i = 0; i < remain; i++)
        WriteByte(0);
    }
  }
  MoveBasePosition(item.PackSize);
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  return S_OK;
}

void COutArchive::WriteCentralHeader(const CItem &item)
{
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);

  bool isUnPack64 = item.UnPackSize >= 0xFFFFFFFF;
  bool isPack64 = item.PackSize >= 0xFFFFFFFF;
  bool isPosition64 = item.LocalHeaderPosition >= 0xFFFFFFFF;
  bool isZip64  = isPack64 || isUnPack64 || isPosition64;
  
  WriteUInt32(NSignature::kCentralFileHeader);
  WriteByte(item.MadeByVersion.Version);
  WriteByte(item.MadeByVersion.HostOS);
  WriteByte(item.ExtractVersion.Version);
  WriteByte(item.ExtractVersion.HostOS);
  WriteUInt16(item.Flags);
  WriteUInt16(item.CompressionMethod);
  WriteUInt32(item.Time);
  WriteUInt32(item.FileCRC);
  WriteUInt32(isPack64 ? 0xFFFFFFFF: (UInt32)item.PackSize);
  WriteUInt32(isUnPack64 ? 0xFFFFFFFF: (UInt32)item.UnPackSize);
  WriteUInt16(item.Name.Length());
  UInt16 zip64ExtraSize = (isUnPack64 ? 8: 0) +  (isPack64 ? 8: 0) + (isPosition64 ? 8: 0);
  UInt16 centralExtraSize = isZip64 ? (4 + zip64ExtraSize) : 0;
  WriteUInt16(centralExtraSize); // test it;
  WriteUInt16(item.Comment.GetCapacity());
  WriteUInt16(0); // DiskNumberStart;
  WriteUInt16(item.InternalAttributes);
  WriteUInt32(item.ExternalAttributes);
  WriteUInt32(isPosition64 ? 0xFFFFFFFF: (UInt32)item.LocalHeaderPosition);
  WriteBytes((const char *)item.Name, item.Name.Length());
  if (isZip64)
  {
    WriteUInt16(0x1); // Zip64 Tag;
    WriteUInt16(zip64ExtraSize);
    if(isUnPack64)
      WriteUInt64(item.UnPackSize);
    if(isPack64)
      WriteUInt64(item.PackSize);
    if(isPosition64)
      WriteUInt64(item.LocalHeaderPosition);
  }
  if (item.Comment.GetCapacity() > 0)
    WriteBytes(item.Comment, item.Comment.GetCapacity());
}

void COutArchive::WriteCentralDir(const CObjectVector<CItem> &items, const CByteBuffer &comment)
{
  m_Stream->Seek(m_BasePosition, STREAM_SEEK_SET, NULL);
  
  UInt64 cdOffset = GetCurrentPosition();
  for(int i = 0; i < items.Size(); i++)
    WriteCentralHeader(items[i]);
  UInt64 cd64EndOffset = GetCurrentPosition();
  UInt64 cdSize = cd64EndOffset - cdOffset;
  bool cdOffset64 = cdOffset >= 0xFFFFFFFF;
  bool cdSize64 = cdSize >= 0xFFFFFFFF;
  bool items64 = items.Size() >= 0xFFFF;
  bool isZip64 = (cdOffset64 || cdSize64 || items64);

  if (isZip64)
  {
    WriteUInt32(NSignature::kZip64EndOfCentralDir);
    WriteUInt64(kZip64EndOfCentralDirRecordSize); // ThisDiskNumber = 0;
    WriteUInt16(45); // version
    WriteUInt16(45); // version
    WriteUInt32(0); // ThisDiskNumber = 0;
    WriteUInt32(0); // StartCentralDirectoryDiskNumber;;
    WriteUInt64((UInt64)items.Size());
    WriteUInt64((UInt64)items.Size());
    WriteUInt64((UInt64)cdSize);
    WriteUInt64((UInt64)cdOffset);

    WriteUInt32(NSignature::kZip64EndOfCentralDirLocator);
    WriteUInt32(0); // number of the disk with the start of the zip64 end of central directory
    WriteUInt64(cd64EndOffset);
    WriteUInt32(1); // total number of disks
  }
  WriteUInt32(NSignature::kEndOfCentralDir);
  WriteUInt16(0); // ThisDiskNumber = 0;
  WriteUInt16(0); // StartCentralDirectoryDiskNumber;
  WriteUInt16(items64 ? 0xFFFF: (UInt16)items.Size());
  WriteUInt16(items64 ? 0xFFFF: (UInt16)items.Size());
  WriteUInt32(cdSize64 ? 0xFFFFFFFF: (UInt32)cdSize);
  WriteUInt32(cdOffset64 ? 0xFFFFFFFF: (UInt32)cdOffset);
  UInt16 commentSize = comment.GetCapacity();
  WriteUInt16(commentSize);
  if (commentSize > 0)
    WriteBytes((const Byte *)comment, commentSize);
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
