// Archive/CabIn.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/MyCom.h"
#include "CabIn.h"
#include "Windows/Defs.h"
#include "../../Common/InBuffer.h"

namespace NArchive{
namespace NCab{

static HRESULT ReadBytes(IInStream *inStream, void *data, UINT32 size)
{
  UINT32 realProcessedSize;
  RINOK(inStream->Read(data, size, &realProcessedSize));
  if(realProcessedSize != size)
    return S_FALSE;
  return S_OK;
}

static HRESULT SafeRead(IInStream *inStream, void *data, UINT32 size)
{
  UINT32 realProcessedSize;
  RINOK(inStream->Read(data, size, &realProcessedSize));
  if(realProcessedSize != size)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
  return S_OK;
}

static void SafeInByteRead(CInBuffer &inBuffer, void *data, UINT32 size)
{
  UINT32 realProcessedSize;
  inBuffer.ReadBytes(data, size, realProcessedSize);
  if(realProcessedSize != size)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
}

static void SafeReadName(CInBuffer &inBuffer, AString &name)
{
  name.Empty();
  while(true)
  {
    BYTE b;
    if (!inBuffer.ReadByte(b))
      throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
    if (b == 0)
      return;
    name += char(b);
  }
}

HRESULT CInArchive::Open(IInStream *inStream, 
    const UINT64 *searchHeaderSizeLimit,
    CInArchiveInfo &inArchiveInfo, 
    CObjectVector<NHeader::CFolder> &folders,
    CObjectVector<CItem> &files,
    CProgressVirt *progressVirt)
{
  UINT64 startPosition;
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &startPosition));

  NHeader::NArchive::CBlock archiveHeader;

  {
    CInBuffer inBuffer;
    inBuffer.Init(inStream);
    UINT64 value = 0;
    const int kSignatureSize = sizeof(value);
    UINT64 kSignature64 = NHeader::NArchive::kSignature;
    while(true)
    {
      BYTE b;
      if (!inBuffer.ReadByte(b))
        return S_FALSE;
      value >>= 8;
      value |= ((UINT64)b) << ((kSignatureSize - 1) * 8);
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
  RINOK(ReadBytes(inStream, &archiveHeader, sizeof(archiveHeader)));
  // if (archiveHeader.Signature != NHeader::NArchive::kSignature)
  //   return S_FALSE;
  
  if (archiveHeader.Reserved1 != 0 ||
      archiveHeader.Reserved2 != 0 ||
      archiveHeader.Reserved3 != 0)
    throw CInArchiveException(CInArchiveException::kUnsupported);

  inArchiveInfo.VersionMinor = archiveHeader.VersionMinor;
  inArchiveInfo.VersionMajor = archiveHeader.VersionMajor;
  inArchiveInfo.NumFolders = archiveHeader.NumFolders;
  inArchiveInfo.NumFiles = archiveHeader.NumFiles;
  inArchiveInfo.Flags = archiveHeader.Flags;

  if (inArchiveInfo.ReserveBlockPresent())
  {
    RINOK(SafeRead(inStream, &inArchiveInfo.PerDataSizes, 
        sizeof(inArchiveInfo.PerDataSizes)));
    RINOK(inStream->Seek(inArchiveInfo.PerDataSizes.PerCabinetAreaSize, 
        STREAM_SEEK_CUR, NULL));
  }

  {
    UINT64 foldersStartPosition;
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &foldersStartPosition));
    CInBuffer inBuffer;
    inBuffer.Init(inStream);
    if ((archiveHeader.Flags & NHeader::NArchive::NFlags::kPrevCabinet) != 0)
    {
      SafeReadName(inBuffer, inArchiveInfo.PreviousCabinetName);
      SafeReadName(inBuffer, inArchiveInfo.PreviousDiskName);
    }
    if ((archiveHeader.Flags & NHeader::NArchive::NFlags::kNextCabinet) != 0)
    {
      SafeReadName(inBuffer, inArchiveInfo.NextCabinetName);
      SafeReadName(inBuffer, inArchiveInfo.NextDiskName);
    }
    foldersStartPosition += inBuffer.GetProcessedSize();
    RINOK(inStream->Seek(foldersStartPosition, STREAM_SEEK_SET, NULL));
  }
  
  if (progressVirt != NULL)
  {
    UINT64 numFiles = archiveHeader.NumFiles;
    RINOK(progressVirt->SetTotal(&numFiles));
  }
  folders.Clear();
  for(int i = 0; i < archiveHeader.NumFolders; i++)
  {
    if (progressVirt != NULL)
    {
      UINT64 numFiles = 0;
      RINOK(progressVirt->SetCompleted(&numFiles));
    }
    NHeader::CFolder folder;
    RINOK(SafeRead(inStream, &folder, sizeof(folder)));
    if (inArchiveInfo.ReserveBlockPresent())
    {
      RINOK(inStream->Seek(
          inArchiveInfo.PerDataSizes.PerFolderAreaSize, STREAM_SEEK_CUR, NULL));
    }
    folder.DataStart += (UINT32)startPosition;
    folders.Add(folder);
  }
  
  RINOK(inStream->Seek(startPosition + archiveHeader.FileOffset, 
      STREAM_SEEK_SET, NULL));

  CInBuffer inBuffer;
  inBuffer.Init(inStream);
  files.Clear();
  if (progressVirt != NULL)
  {
    UINT64 numFiles = files.Size();
    RINOK(progressVirt->SetCompleted(&numFiles));
  }
  for(i = 0; i < archiveHeader.NumFiles; i++)
  {
    NHeader::CFile fileHeader;
    SafeInByteRead(inBuffer, &fileHeader, sizeof(fileHeader));
    CItem item;
    item.UnPackSize = fileHeader.UnPackSize;
    item.UnPackOffset = fileHeader.UnPackOffset;
    item.FolderIndex = fileHeader.FolderIndex;
    item.Time = ((UINT32(fileHeader.PureDate) << 16)) | fileHeader.PureTime;
    item.Attributes = fileHeader.Attributes;
    SafeReadName(inBuffer, item.Name);
    files.Add(item);
    if (progressVirt != NULL)
    {
      UINT64 numFiles = files.Size();
      RINOK(progressVirt->SetCompleted(&numFiles));
    }
  }
  return S_OK;
}

}}
