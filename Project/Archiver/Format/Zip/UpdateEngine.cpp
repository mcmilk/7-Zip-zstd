// UpdateArchiveEngine.cpp

#include "StdAfx.h"

#include "UpdateEngine.h"

#include "Compression/CopyCoder.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"

#include "Windows/Defs.h"

#include "AddCommon.h"
#include "Handler.h"

#include "../Common/InStreamWithCRC.h"

#include "Interface/LimitedStreams.h"

using namespace std;

namespace NArchive {
namespace NZip {

static const kOneItemComplexity = 30;

static const BYTE kMadeByHostOS = NFileHeader::NHostOS::kFAT;
static const BYTE kExtractHostOS = NFileHeader::NHostOS::kFAT;

static const BYTE kMethodForDirectory = NFileHeader::NCompressionMethod::kStored;
static const BYTE kExtractVersionForDirectory = NFileHeader::NCompressionMethod::kStoreExtractVersion;

static HRESULT CopyBlock(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> copyCoder = copyCoderSpec;
  return copyCoder->Code(inStream, outStream, NULL, NULL, progress);
}

HRESULT CopyBlockToArchive(ISequentialInStream *inStream, 
    COutArchive &outArchive, ICompressProgressInfo *progress)
{
  CComPtr<ISequentialOutStream> outStream;
  outArchive.CreateStreamForCopying(&outStream);
  return CopyBlock(inStream, outStream, progress);
}

static HRESULT WriteRange(IInStream *inStream, 
    COutArchive &outArchive, 
    const CUpdateRange &range, 
    IProgress *progress,
    UINT64 &currentComplexity)
{
  UINT64 position;
  inStream->Seek(range.Position, STREAM_SEEK_SET, &position);

  CComObjectNoLock<CLimitedSequentialInStream> *streamSpec = new 
      CComObjectNoLock<CLimitedSequentialInStream>;
  CComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
  streamSpec->Init(inStream, range.Size);


  CComObjectNoLock<CLocalProgress> *localProgressSpec = new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(progress, true);
  
  CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
  CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

  localCompressProgressSpec->Init(localProgress, &currentComplexity, &currentComplexity);

  HRESULT result = CopyBlockToArchive(inStreamLimited, outArchive, compressProgress);
  currentComplexity += range.Size;
  return result;
}


static HRESULT UpdateOneFile(IInStream *inStream, 
    COutArchive &archive, 
    const CCompressionMethodMode &options,
    CAddCommon &compressor, 
    const CUpdateItemInfo &updateItem, 
    UINT64 &currentComplexity,
    IArchiveUpdateCallback *updateCallback,
    CItemInfoEx &fileHeader)
{
  CComPtr<IInStream> fileInStream;
  RINOK(updateCallback->GetStream(updateItem.IndexInClient, &fileInStream));

  bool isDirectory;
  UINT32 fileSize = updateItem.Size;

  if (updateItem.NewProperties)
  {
    isDirectory = updateItem.IsDirectory;
    fileHeader.Name = updateItem.Name; 
    fileHeader.ExternalAttributes = updateItem.Attributes;
    fileHeader.Time = updateItem.Time;
  }
  else
    isDirectory = fileHeader.IsDirectory();

  archive.PrepareWriteCompressedData(fileHeader.Name.Length());

  fileHeader.LocalHeaderPosition = archive.GetCurrentPosition();
  fileHeader.MadeByVersion.HostOS = kMadeByHostOS;
  fileHeader.MadeByVersion.Version = NFileHeader::NCompressionMethod::kMadeByProgramVersion;
  
  fileHeader.ExtractVersion.HostOS = kExtractHostOS;

  fileHeader.InternalAttributes = 0; // test it
  fileHeader.ClearFlags();
  if(isDirectory)
  {
    fileHeader.ExtractVersion.Version = kExtractVersionForDirectory;
    fileHeader.CompressionMethod = kMethodForDirectory;

    fileHeader.PackSize = 0;
    fileHeader.FileCRC = 0; // test it
  }
  else
  {
    {
      CComObjectNoLock<CInStreamWithCRC> *inStreamSpec = 
        new CComObjectNoLock<CInStreamWithCRC>;
      CComPtr<IInStream> inStream(inStreamSpec);
      inStreamSpec->Init(fileInStream);
      CCompressingResult compressingResult;
      CComPtr<IOutStream> outStream;
      archive.CreateStreamForCompressing(&outStream);

      CComObjectNoLock<CLocalProgress> *localProgressSpec = 
          new  CComObjectNoLock<CLocalProgress>;
      CComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
      localProgressSpec->Init(updateCallback, true);
  
      CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
          new  CComObjectNoLock<CLocalCompressProgressInfo>;
      CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

      localCompressProgressSpec->Init(localProgress, &currentComplexity, NULL);

      RINOK(compressor.Compress(inStream, outStream, 
          fileSize, compressProgress, compressingResult));

      fileHeader.PackSize = compressingResult.PackSize;
      fileHeader.CompressionMethod = compressingResult.Method;
      fileHeader.ExtractVersion.Version = compressingResult.ExtractVersion;
      fileHeader.FileCRC = inStreamSpec->GetCRC();
    }
  }
  fileHeader.UnPackSize = fileSize;
  fileHeader.SetEncrypted(options.PasswordIsDefined);
  fileHeader.CommentSize = (updateItem.Commented) ? 
      WORD(updateItem.CommentRange.Size) : 0;

  fileHeader.LocalExtraSize = 0;
  fileHeader.CentralExtraSize = 0;

  archive.WriteLocalHeader(fileHeader);
  currentComplexity += fileSize;
  return updateCallback->SetOperationResult(
      NArchive::NUpdate::NOperationResult::kOK);
}

HRESULT UpdateArchiveStd(COutArchive &archive, 
    IInStream *inStream,
    const NArchive::NZip::CItemInfoExVector &inputItems,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    const CCompressionMethodMode *options, 
    bool commentRangeAssigned,
    const CUpdateRange &commentRange,
    IArchiveUpdateCallback *updateCallback)
{
  UINT64 complexity = 0;
  
  int i;
  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItemInfo &updateItem = updateItems[i];
    if (updateItem.NewData)
    {
      complexity += updateItem.Size;
      if (updateItem.Commented)
        complexity += updateItem.CommentRange.Size;
    }
    else
    {
      const CItemInfoEx &inputItem = inputItems[updateItem.IndexInArchive];
      complexity += inputItem.GetLocalFullSize();
      complexity += inputItem.GetCentralExtraPlusCommentSize();
    }
    complexity += kOneItemComplexity * 2;
  }
  if (commentRangeAssigned)
    complexity += commentRange.Size;

  complexity++; // end of central
  
  updateCallback->SetTotal(complexity);
  auto_ptr<CAddCommon> compressor;
  
  complexity = 0;
  
  CItemInfoExVector items;

  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItemInfo &updateItem = updateItems[i];
    RINOK(updateCallback->SetCompleted(&complexity));

    CItemInfoEx item;
    if (!updateItem.NewProperties || !updateItem.NewData)
      item = inputItems[updateItem.IndexInArchive];

    if (updateItem.NewData)
    {
      if(compressor.get() == NULL)
      {
        compressor = auto_ptr<CAddCommon>(new CAddCommon(*options));
      }
      RINOK(UpdateOneFile(inStream, archive, *options, 
          *compressor, updateItem, complexity, updateCallback, item));
    }
    else
    {
      // item = inputItems[copyIndices[copyIndexIndex++]];
      if (updateItem.NewProperties)
      {
        if (item.HasDescriptor())
          return E_NOTIMPL;
        
        // use old name size.
        // CUpdateRange range(item.GetLocalExtraPosition(), item.LocalExtraSize + item.PackSize);
        CUpdateRange range(item.GetDataPosition(), item.PackSize);

        // item.ExternalAttributes = updateItem.Attributes;
        // Test it
        item.Name = updateItem.Name; 
        item.Time = updateItem.Time;
        item.CentralExtraSize = 0;
        item.LocalExtraSize = 0;

        archive.PrepareWriteCompressedData(item.Name.Length());
        item.LocalHeaderPosition = archive.GetCurrentPosition();
        archive.SeekToPackedDataPosition();
        RINOK(WriteRange(inStream, archive, range, 
          updateCallback, complexity));
        archive.WriteLocalHeader(item);
      }
      else
      {
        CUpdateRange range(item.LocalHeaderPosition, item.GetLocalFullSize());
      
        // set new header position
        item.LocalHeaderPosition = archive.GetCurrentPosition(); 
        
        RINOK(WriteRange(inStream, archive, range, 
          updateCallback, complexity));
        archive.MoveBasePosition(range.Size);
      }
    }
    items.Add(item);
    complexity += kOneItemComplexity;
  }
  DWORD centralDirStartPosition = archive.GetCurrentPosition();
  for(i = 0; i < items.Size(); i++)
  {
    archive.WriteCentralHeader(items[i]);
    const CUpdateItemInfo &updateItem = updateItems[i];
    if (updateItem.NewProperties)
    {
      if (updateItem.Commented)
      {
        const CUpdateRange range = updateItem.CommentRange;
        RINOK(WriteRange(inStream, archive, range, 
            updateCallback, complexity));
        archive.MoveBasePosition(range.Size);
      }
    }
    else
    {
      const CItemInfoEx item = items[i];
      CUpdateRange range(item.CentralExtraPosition, 
        item.GetCentralExtraPlusCommentSize());
      RINOK(WriteRange(inStream, archive, range, 
          updateCallback, complexity));
      archive.MoveBasePosition(range.Size);
    }
    complexity += kOneItemComplexity;
  }
  COutArchiveInfo archiveInfo;
  archiveInfo.NumEntriesInCentaralDirectory = items.Size();
  archiveInfo.CentralDirectorySize = archive.GetCurrentPosition() - 
      centralDirStartPosition;
  archiveInfo.CentralDirectoryStartOffset = centralDirStartPosition;
  archiveInfo.CommentSize = commentRangeAssigned ? WORD(commentRange.Size) : 0;
  archive.WriteEndOfCentralDir(archiveInfo);
  if (commentRangeAssigned)
     RINOK(WriteRange(inStream, archive, commentRange,
        updateCallback, complexity));
  complexity++; // end of central
  return S_OK;
}

}}
