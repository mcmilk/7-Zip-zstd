// ZipOut.cpp

#include "StdAfx.h"

#include "../../Common/OffsetStream.h"

#include "ZipOut.h"

namespace NArchive {
namespace NZip {

void COutArchive::Create(IOutStream *outStream)
{
  if (!m_OutBuffer.Create(1 << 16))
    throw CSystemException(E_OUTOFMEMORY);
  m_Stream = outStream;
  m_OutBuffer.SetStream(outStream);
  m_OutBuffer.Init();
  m_BasePosition = 0;
}

void COutArchive::MoveBasePosition(UInt64 distanceToMove)
{
  m_BasePosition += distanceToMove; // test overflow
}

void COutArchive::PrepareWriteCompressedDataZip64(UInt16 fileNameLength, bool isZip64, bool aesEncryption)
{
  m_IsZip64 = isZip64;
  m_ExtraSize = isZip64 ? (4 + 8 + 8) : 0;
  if (aesEncryption)
    m_ExtraSize += 4 + 7;
  m_LocalFileHeaderSize = 4 + NFileHeader::kLocalBlockSize + fileNameLength + m_ExtraSize;
}

void COutArchive::PrepareWriteCompressedData(UInt16 fileNameLength, UInt64 unPackSize, bool aesEncryption)
{
  // We test it to 0xF8000000 to support case when compressed size
  // can be larger than uncompressed size.
  PrepareWriteCompressedDataZip64(fileNameLength, unPackSize >= 0xF8000000, aesEncryption);
}

void COutArchive::PrepareWriteCompressedData2(UInt16 fileNameLength, UInt64 unPackSize, UInt64 packSize, bool aesEncryption)
{
  bool isUnPack64 = unPackSize >= 0xFFFFFFFF;
  bool isPack64 = packSize >= 0xFFFFFFFF;
  bool isZip64 = isPack64 || isUnPack64;
  PrepareWriteCompressedDataZip64(fileNameLength, isZip64, aesEncryption);
}

void COutArchive::WriteBytes(const void *buffer, UInt32 size)
{
  m_OutBuffer.WriteBytes(buffer, size);
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

void COutArchive::WriteExtra(const CExtraBlock &extra)
{
  if (extra.SubBlocks.Size() != 0)
  {
    for (int i = 0; i < extra.SubBlocks.Size(); i++)
    {
      const CExtraSubBlock &subBlock = extra.SubBlocks[i];
      WriteUInt16(subBlock.ID);
      WriteUInt16((UInt16)subBlock.Data.GetCapacity());
      WriteBytes(subBlock.Data, (UInt32)subBlock.Data.GetCapacity());
    }
  }
}

void COutArchive::SeekTo(UInt64 offset)
{
  HRESULT res = m_Stream->Seek(offset, STREAM_SEEK_SET, NULL);
  if (res != S_OK)
    throw CSystemException(res);
}

void COutArchive::WriteLocalHeader(const CLocalItem &item)
{
  SeekTo(m_BasePosition);
  
  bool isZip64 = m_IsZip64 || item.PackSize >= 0xFFFFFFFF || item.UnPackSize >= 0xFFFFFFFF;
  
  WriteUInt32(NSignature::kLocalFileHeader);
  {
    Byte ver = item.ExtractVersion.Version;
    if (isZip64 && ver < NFileHeader::NCompressionMethod::kExtractVersion_Zip64)
      ver = NFileHeader::NCompressionMethod::kExtractVersion_Zip64;
    WriteByte(ver);
  }
  WriteByte(item.ExtractVersion.HostOS);
  WriteUInt16(item.Flags);
  WriteUInt16(item.CompressionMethod);
  WriteUInt32(item.Time);
  WriteUInt32(item.FileCRC);
  WriteUInt32(isZip64 ? 0xFFFFFFFF: (UInt32)item.PackSize);
  WriteUInt32(isZip64 ? 0xFFFFFFFF: (UInt32)item.UnPackSize);
  WriteUInt16((UInt16)item.Name.Length());
  {
    UInt16 localExtraSize = (UInt16)((isZip64 ? (4 + 16): 0) + item.LocalExtra.GetSize());
    if (localExtraSize > m_ExtraSize)
      throw CSystemException(E_FAIL);
  }
  WriteUInt16((UInt16)m_ExtraSize); // test it;
  WriteBytes((const char *)item.Name, item.Name.Length());

  UInt32 extraPos = 0;
  if (isZip64)
  {
    extraPos += 4 + 16;
    WriteUInt16(NFileHeader::NExtraID::kZip64);
    WriteUInt16(16);
    WriteUInt64(item.UnPackSize);
    WriteUInt64(item.PackSize);
  }

  WriteExtra(item.LocalExtra);
  extraPos += (UInt32)item.LocalExtra.GetSize();
  for (; extraPos < m_ExtraSize; extraPos++)
    WriteByte(0);

  m_OutBuffer.FlushWithCheck();
  MoveBasePosition(item.PackSize);
  SeekTo(m_BasePosition);
}

void COutArchive::WriteCentralHeader(const CItem &item)
{
  bool isUnPack64 = item.UnPackSize >= 0xFFFFFFFF;
  bool isPack64 = item.PackSize >= 0xFFFFFFFF;
  bool isPosition64 = item.LocalHeaderPosition >= 0xFFFFFFFF;
  bool isZip64  = isPack64 || isUnPack64 || isPosition64;
  
  WriteUInt32(NSignature::kCentralFileHeader);
  WriteByte(item.MadeByVersion.Version);
  WriteByte(item.MadeByVersion.HostOS);
  {
    Byte ver = item.ExtractVersion.Version;
    if (isZip64 && ver < NFileHeader::NCompressionMethod::kExtractVersion_Zip64)
      ver = NFileHeader::NCompressionMethod::kExtractVersion_Zip64;
    WriteByte(ver);
  }
  WriteByte(item.ExtractVersion.HostOS);
  WriteUInt16(item.Flags);
  WriteUInt16(item.CompressionMethod);
  WriteUInt32(item.Time);
  WriteUInt32(item.FileCRC);
  WriteUInt32(isPack64 ? 0xFFFFFFFF: (UInt32)item.PackSize);
  WriteUInt32(isUnPack64 ? 0xFFFFFFFF: (UInt32)item.UnPackSize);
  WriteUInt16((UInt16)item.Name.Length());
  UInt16 zip64ExtraSize = (UInt16)((isUnPack64 ? 8: 0) +  (isPack64 ? 8: 0) + (isPosition64 ? 8: 0));
  const UInt16 kNtfsExtraSize = 4 + 2 + 2 + (3 * 8);
  UInt16 centralExtraSize = (UInt16)(isZip64 ? (4 + zip64ExtraSize) : 0) + (item.NtfsTimeIsDefined ? (4 + kNtfsExtraSize) : 0);
  centralExtraSize = (UInt16)(centralExtraSize + item.CentralExtra.GetSize());
  WriteUInt16(centralExtraSize); // test it;
  WriteUInt16((UInt16)item.Comment.GetCapacity());
  WriteUInt16(0); // DiskNumberStart;
  WriteUInt16(item.InternalAttributes);
  WriteUInt32(item.ExternalAttributes);
  WriteUInt32(isPosition64 ? 0xFFFFFFFF: (UInt32)item.LocalHeaderPosition);
  WriteBytes((const char *)item.Name, item.Name.Length());
  if (isZip64)
  {
    WriteUInt16(NFileHeader::NExtraID::kZip64);
    WriteUInt16(zip64ExtraSize);
    if(isUnPack64)
      WriteUInt64(item.UnPackSize);
    if(isPack64)
      WriteUInt64(item.PackSize);
    if(isPosition64)
      WriteUInt64(item.LocalHeaderPosition);
  }
  if (item.NtfsTimeIsDefined)
  {
    WriteUInt16(NFileHeader::NExtraID::kNTFS);
    WriteUInt16(kNtfsExtraSize);
    WriteUInt32(0); // reserved
    WriteUInt16(NFileHeader::NNtfsExtra::kTagTime);
    WriteUInt16(8 * 3);
    WriteUInt32(item.NtfsMTime.dwLowDateTime);
    WriteUInt32(item.NtfsMTime.dwHighDateTime);
    WriteUInt32(item.NtfsATime.dwLowDateTime);
    WriteUInt32(item.NtfsATime.dwHighDateTime);
    WriteUInt32(item.NtfsCTime.dwLowDateTime);
    WriteUInt32(item.NtfsCTime.dwHighDateTime);
  }
  WriteExtra(item.CentralExtra);
  if (item.Comment.GetCapacity() > 0)
    WriteBytes(item.Comment, (UInt32)item.Comment.GetCapacity());
}

void COutArchive::WriteCentralDir(const CObjectVector<CItem> &items, const CByteBuffer *comment)
{
  SeekTo(m_BasePosition);
  
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
    WriteUInt64(kZip64EcdSize); // ThisDiskNumber = 0;
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
  WriteUInt16((UInt16)(items64 ? 0xFFFF: items.Size()));
  WriteUInt16((UInt16)(items64 ? 0xFFFF: items.Size()));
  WriteUInt32(cdSize64 ? 0xFFFFFFFF: (UInt32)cdSize);
  WriteUInt32(cdOffset64 ? 0xFFFFFFFF: (UInt32)cdOffset);
  UInt32 commentSize = (UInt32)(comment ? comment->GetCapacity() : 0);
  WriteUInt16((UInt16)commentSize);
  if (commentSize > 0)
    WriteBytes((const Byte *)*comment, commentSize);
  m_OutBuffer.FlushWithCheck();
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
  SeekTo(m_BasePosition + m_LocalFileHeaderSize);
}

void COutArchive::CreateStreamForCopying(ISequentialOutStream **outStream)
{
  CMyComPtr<ISequentialOutStream> tempStream(m_Stream);
  *outStream = tempStream.Detach();
}

}}
