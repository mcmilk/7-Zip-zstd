// Archive/Cab/InEngine.cpp

#include "StdAfx.h"

//#include "WinConvert.h"
#include "Common/StringConvert.h"
#include "Archive/Cab/InEngine.h"
#include "Windows/Defs.h"
#include "Stream/InByte.h"

namespace NArchive{
namespace NCab{

static HRESULT ReadBytes(IInStream *aStream, void *aData, UINT32 aSize)
{
  UINT32 aProcessedSizeReal;
  RETURN_IF_NOT_S_OK(aStream->Read(aData, aSize, &aProcessedSizeReal));
  if(aProcessedSizeReal != aSize)
    return S_FALSE;
  return S_OK;
}

static HRESULT SafeRead(IInStream *aStream, void *aData, UINT32 aSize)
{
  UINT32 aProcessedSizeReal;
  RETURN_IF_NOT_S_OK(aStream->Read(aData, aSize, &aProcessedSizeReal));
  if(aProcessedSizeReal != aSize)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
  return S_OK;
}

static void SafeInByteRead(NStream::CInByte &anInByte, void *aData, UINT32 aSize)
{
  UINT32 aProcessedSizeReal;
  anInByte.ReadBytes(aData, aSize, aProcessedSizeReal);
  if(aProcessedSizeReal != aSize)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
}

static void SafeReadName(NStream::CInByte &anInByte, AString &aName)
{
  aName.Empty();
  while(true)
  {
    BYTE aByte;
    if (!anInByte.ReadByte(aByte))
      throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
    if (aByte == 0)
      return;
    aName += char(aByte);
  }
}

HRESULT CInArchive::Open(IInStream *aStream, 
    const UINT64 *aSearchHeaderSizeLimit,
    CInArchiveInfo &anInArchiveInfo, 
    CObjectVector<NHeader::CFolder> &aFolders,
    CObjectVector<CFileInfo> &aFiles,
    CProgressVirt *aProgressVirt)
{
  UINT64 aStartPosition;
  RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &aStartPosition));

  NHeader::NArchive::CBlock anArchiveHeader;

  {
    NStream::CInByte anInByte;
    anInByte.Init(aStream);
    UINT64 aValue = 0;
    const kSignatureSize = sizeof(aValue);
    UINT64 kSignature64 = NHeader::NArchive::kSignature;
    while(true)
    {
      BYTE aByte;
      if (!anInByte.ReadByte(aByte))
        return S_FALSE;
      aValue >>= 8;
      aValue |= ((UINT64)aByte) << ((kSignatureSize - 1) * 8);
      if (anInByte.GetProcessedSize() >= kSignatureSize)
      {
        if (aValue == kSignature64)
          break;
        if (aSearchHeaderSizeLimit != NULL)
          if (anInByte.GetProcessedSize() > (*aSearchHeaderSizeLimit))
            return S_FALSE;
      }
    }
    aStartPosition += anInByte.GetProcessedSize() - kSignatureSize;
  }
  RETURN_IF_NOT_S_OK(aStream->Seek(aStartPosition, STREAM_SEEK_SET, NULL));
  RETURN_IF_NOT_S_OK(ReadBytes(aStream, &anArchiveHeader, sizeof(anArchiveHeader)));
  // if (anArchiveHeader.Signature != NHeader::NArchive::kSignature)
  //   return S_FALSE;
  
  if (anArchiveHeader.Reserved1 != 0 ||
      anArchiveHeader.Reserved2 != 0 ||
      anArchiveHeader.Reserved3 != 0)
    throw CInArchiveException(CInArchiveException::kUnsupported);

  anInArchiveInfo.VersionMinor = anArchiveHeader.VersionMinor;
  anInArchiveInfo.VersionMajor = anArchiveHeader.VersionMajor;
  anInArchiveInfo.NumFolders = anArchiveHeader.NumFolders;
  anInArchiveInfo.NumFiles = anArchiveHeader.NumFiles;
  anInArchiveInfo.Flags = anArchiveHeader.Flags;

  if (anInArchiveInfo.ReserveBlockPresent())
  {
    RETURN_IF_NOT_S_OK(SafeRead(aStream, &anInArchiveInfo.PerDataSizes, 
        sizeof(anInArchiveInfo.PerDataSizes)));
    RETURN_IF_NOT_S_OK(aStream->Seek(anInArchiveInfo.PerDataSizes.PerCabinetAreaSize, 
        STREAM_SEEK_CUR, NULL));
  }

  {
    UINT64 aFoldersStartPosition;
    RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &aFoldersStartPosition));
    NStream::CInByte anInByte;
    anInByte.Init(aStream);
    if ((anArchiveHeader.Flags & NHeader::NArchive::NFlags::kPrevCabinet) != 0)
    {
      SafeReadName(anInByte, anInArchiveInfo.PreviousCabinetName);
      SafeReadName(anInByte, anInArchiveInfo.PreviousDiskName);
    }
    if ((anArchiveHeader.Flags & NHeader::NArchive::NFlags::kNextCabinet) != 0)
    {
      SafeReadName(anInByte, anInArchiveInfo.NextCabinetName);
      SafeReadName(anInByte, anInArchiveInfo.NextDiskName);
    }
    aFoldersStartPosition += anInByte.GetProcessedSize();
    RETURN_IF_NOT_S_OK(aStream->Seek(aFoldersStartPosition, STREAM_SEEK_SET, NULL));
  }
  
  if (aProgressVirt != NULL)
  {
    UINT64 aNumFiles = anArchiveHeader.NumFiles;
    RETURN_IF_NOT_S_OK(aProgressVirt->SetTotal(&aNumFiles));
  }
  aFolders.Clear();
  for(int i = 0; i < anArchiveHeader.NumFolders; i++)
  {
    if (aProgressVirt != NULL)
    {
      UINT64 aNumFiles = 0;
      RETURN_IF_NOT_S_OK(aProgressVirt->SetCompleted(&aNumFiles));
    }
    NHeader::CFolder aFolder;
    RETURN_IF_NOT_S_OK(SafeRead(aStream, &aFolder, sizeof(aFolder)));
    if (anInArchiveInfo.ReserveBlockPresent())
    {
      RETURN_IF_NOT_S_OK(aStream->Seek(
          anInArchiveInfo.PerDataSizes.PerFolderAreaSize, STREAM_SEEK_CUR, NULL));
    }
    aFolder.DataStart += aStartPosition;
    aFolders.Add(aFolder);
  }
  
  RETURN_IF_NOT_S_OK(aStream->Seek(aStartPosition + anArchiveHeader.FileOffset, 
      STREAM_SEEK_SET, NULL));

  NStream::CInByte anInByte;
  anInByte.Init(aStream);
  aFiles.Clear();
  if (aProgressVirt != NULL)
  {
    UINT64 aNumFiles = aFiles.Size();
    RETURN_IF_NOT_S_OK(aProgressVirt->SetCompleted(&aNumFiles));
  }
  for(i = 0; i < anArchiveHeader.NumFiles; i++)
  {
    NHeader::CFile aFileHeader;
    SafeInByteRead(anInByte, &aFileHeader, sizeof(aFileHeader));
    CFileInfo aFileInfo;
    aFileInfo.UnPackSize = aFileHeader.UnPackSize;
    aFileInfo.UnPackOffset = aFileHeader.UnPackOffset;
    aFileInfo.FolderIndex = aFileHeader.FolderIndex;
    aFileInfo.Time = ((UINT32(aFileHeader.PureDate) << 16)) | aFileHeader.PureTime;
    aFileInfo.Attributes = aFileHeader.Attributes;
    SafeReadName(anInByte, aFileInfo.Name);
    aFiles.Add(aFileInfo);
    if (aProgressVirt != NULL)
    {
      UINT64 aNumFiles = aFiles.Size();
      RETURN_IF_NOT_S_OK(aProgressVirt->SetCompleted(&aNumFiles));
    }
  }
  return S_OK;
}

}}
