// Archive/CabIn.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/MyCom.h"
#include "CabIn.h"
#include "Windows/Defs.h"
#include "../../Common/InBuffer.h"

namespace NArchive{
namespace NCab{

static HRESULT ReadBytes(IInStream *inStream, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  RINOK(inStream->Read(data, size, &realProcessedSize));
  if(realProcessedSize != size)
    return S_FALSE;
  return S_OK;
}

static HRESULT SafeRead(IInStream *inStream, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  RINOK(inStream->Read(data, size, &realProcessedSize));
  if(realProcessedSize != size)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
  return S_OK;
}

static void SafeInByteRead(::CInBuffer &inBuffer, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  inBuffer.ReadBytes(data, size, realProcessedSize);
  if(realProcessedSize != size)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
}

static void SafeReadName(::CInBuffer &inBuffer, AString &name)
{
  name.Empty();
  while(true)
  {
    Byte b;
    if (!inBuffer.ReadByte(b))
      throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
    if (b == 0)
      return;
    name += char(b);
  }
}

Byte CInArchive::ReadByte()
{
  if (_blockPos >= _blockSize)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
  return _block[_blockPos++];
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

HRESULT CInArchive::Open(IInStream *inStream, 
    const UInt64 *searchHeaderSizeLimit,
    CInArchiveInfo &inArchiveInfo, 
    CObjectVector<NHeader::CFolder> &folders,
    CObjectVector<CItem> &files,
    CProgressVirt *progressVirt)
{
  UInt64 startPosition;
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &startPosition));

  // NHeader::NArchive::CBlock archiveHeader;

  {
    ::CInBuffer inBuffer;
    if (!inBuffer.Create(1 << 17))
      return E_OUTOFMEMORY;
    inBuffer.SetStream(inStream);
    inBuffer.Init();
    UInt64 value = 0;
    const int kSignatureSize = 8;
    UInt64 kSignature64 = NHeader::NArchive::kSignature;
    while(true)
    {
      Byte b;
      if (!inBuffer.ReadByte(b))
        return S_FALSE;
      value >>= 8;
      value |= ((UInt64)b) << ((kSignatureSize - 1) * 8);
      if (inBuffer.GetProcessedSize() >= kSignatureSize)
      {
        if (value == kSignature64)
          break;
        if (searchHeaderSizeLimit != NULL)
          if (inBuffer.GetProcessedSize() > (*searchHeaderSizeLimit))
            return S_FALSE;
      }
    }
    startPosition += inBuffer.GetProcessedSize() - kSignatureSize;
  }
  RINOK(inStream->Seek(startPosition, STREAM_SEEK_SET, NULL));

  RINOK(ReadBytes(inStream, _block, NHeader::NArchive::kArchiveHeaderSize));
  _blockPos = 0;

  ReadUInt32(); // Signature;	// cabinet file signature
  // if (archiveHeader.Signature != NHeader::NArchive::kSignature)
  //   return S_FALSE;

  UInt32 reserved1 = ReadUInt32();
  UInt32 size = ReadUInt32();	// size of this cabinet file in bytes
  UInt32 reserved2 = ReadUInt32();
  UInt32 fileOffset = ReadUInt32();	// offset of the first CFFILE entry
  UInt32 reserved3 = ReadUInt32();

  inArchiveInfo.VersionMinor = ReadByte();	// cabinet file format version, minor
  inArchiveInfo.VersionMajor = ReadByte();	// cabinet file format version, major
  inArchiveInfo.NumFolders = ReadUInt16();	// number of CFFOLDER entries in this cabinet
  inArchiveInfo.NumFiles  = ReadUInt16();	// number of CFFILE entries in this cabinet
  inArchiveInfo.Flags = ReadUInt16();	// number of CFFILE entries in this cabinet
  UInt16 setID = ReadUInt16();	// must be the same for all cabinets in a set
  UInt16 cabinetNumber = ReadUInt16();	// number of this cabinet file in a set

  if (reserved1 != 0 || reserved2 != 0 || reserved3 != 0)
    throw CInArchiveException(CInArchiveException::kUnsupported);

  if (inArchiveInfo.ReserveBlockPresent())
  {
    RINOK(SafeRead(inStream, _block, 
        NHeader::NArchive::kPerDataSizesHeaderSize));
    _blockPos = 0;

    inArchiveInfo.PerDataSizes.PerCabinetAreaSize = ReadUInt16();	// (optional) size of per-cabinet reserved area
    inArchiveInfo.PerDataSizes.PerFolderAreaSize = ReadByte();	// (optional) size of per-folder reserved area
    inArchiveInfo.PerDataSizes.PerDatablockAreaSize = ReadByte();	// (optional) size of per-datablock reserved area
    RINOK(inStream->Seek(inArchiveInfo.PerDataSizes.PerCabinetAreaSize, 
        STREAM_SEEK_CUR, NULL));
  }

  {
    UInt64 foldersStartPosition;
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &foldersStartPosition));
    ::CInBuffer inBuffer;
    if (!inBuffer.Create(1 << 17))
      return E_OUTOFMEMORY;
    inBuffer.SetStream(inStream);
    inBuffer.Init();
    if ((inArchiveInfo.Flags & NHeader::NArchive::NFlags::kPrevCabinet) != 0)
    {
      SafeReadName(inBuffer, inArchiveInfo.PreviousCabinetName);
      SafeReadName(inBuffer, inArchiveInfo.PreviousDiskName);
    }
    if ((inArchiveInfo.Flags & NHeader::NArchive::NFlags::kNextCabinet) != 0)
    {
      SafeReadName(inBuffer, inArchiveInfo.NextCabinetName);
      SafeReadName(inBuffer, inArchiveInfo.NextDiskName);
    }
    foldersStartPosition += inBuffer.GetProcessedSize();
    RINOK(inStream->Seek(foldersStartPosition, STREAM_SEEK_SET, NULL));
  }
  
  if (progressVirt != NULL)
  {
    UInt64 numFiles = inArchiveInfo.NumFiles;
    RINOK(progressVirt->SetTotal(&numFiles));
  }
  folders.Clear();
  for(int i = 0; i < inArchiveInfo.NumFolders; i++)
  {
    if (progressVirt != NULL)
    {
      UInt64 numFiles = 0;
      RINOK(progressVirt->SetCompleted(&numFiles));
    }
    NHeader::CFolder folder;
    RINOK(SafeRead(inStream, _block, NHeader::kFolderHeaderSize));
    _blockPos = 0;

    folder.DataStart = ReadUInt32();
    folder.NumDataBlocks = ReadUInt16();
    folder.CompressionTypeMajor = ReadByte();
    folder.CompressionTypeMinor = ReadByte();

    if (inArchiveInfo.ReserveBlockPresent())
    {
      RINOK(inStream->Seek(
          inArchiveInfo.PerDataSizes.PerFolderAreaSize, STREAM_SEEK_CUR, NULL));
    }
    folder.DataStart += (UInt32)startPosition;
    folders.Add(folder);
  }
  
  RINOK(inStream->Seek(startPosition + fileOffset, 
      STREAM_SEEK_SET, NULL));

  ::CInBuffer inBuffer;
  if (!inBuffer.Create(1 << 17))
    return E_OUTOFMEMORY;
  inBuffer.SetStream(inStream);
  inBuffer.Init();
  files.Clear();
  if (progressVirt != NULL)
  {
    UInt64 numFiles = files.Size();
    RINOK(progressVirt->SetCompleted(&numFiles));
  }
  for(i = 0; i < inArchiveInfo.NumFiles; i++)
  {
    SafeInByteRead(inBuffer, _block, NHeader::kFileHeaderSize);
    _blockPos = 0;
    CItem item;
    item.UnPackSize = ReadUInt32();
    item.UnPackOffset = ReadUInt32();
    item.FolderIndex = ReadUInt16();
    UInt16 pureDate = ReadUInt16();
    UInt16 pureTime = ReadUInt16();
    item.Time = ((UInt32(pureDate) << 16)) | pureTime;
    item.Attributes = ReadUInt16();
    SafeReadName(inBuffer, item.Name);
    files.Add(item);
    if (progressVirt != NULL)
    {
      UInt64 numFiles = files.Size();
      RINOK(progressVirt->SetCompleted(&numFiles));
    }
  }
  return S_OK;
}

}}
