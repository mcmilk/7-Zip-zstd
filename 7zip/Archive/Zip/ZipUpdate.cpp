// ZipUpdate.cpp

#include "StdAfx.h"

#include "ZipUpdate.h"
#include "ZipAddCommon.h"
#include "ZipOut.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Windows/Defs.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"
#include "../../Compress/Copy/CopyCoder.h"

#include "../Common/InStreamWithCRC.h"

using namespace std;

namespace NArchive {
namespace NZip {

static const BYTE kMadeByHostOS = NFileHeader::NHostOS::kFAT;
static const BYTE kExtractHostOS = NFileHeader::NHostOS::kFAT;

static const BYTE kMethodForDirectory = NFileHeader::NCompressionMethod::kStored;
static const BYTE kExtractVersionForDirectory = NFileHeader::NCompressionMethod::kStoreExtractVersion;

static HRESULT CopyBlock(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, progress);
}

HRESULT CopyBlockToArchive(ISequentialInStream *inStream, 
    COutArchive &outArchive, ICompressProgressInfo *progress)
{
  CMyComPtr<ISequentialOutStream> outStream;
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

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
  streamSpec->Init(inStream, range.Size);

  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(progress, true);
  
  CLocalCompressProgressInfo *localCompressProgressSpec = new CLocalCompressProgressInfo;
  CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

  localCompressProgressSpec->Init(localProgress, &currentComplexity, &currentComplexity);

  HRESULT result = CopyBlockToArchive(inStreamLimited, outArchive, compressProgress);
  currentComplexity += range.Size;
  return result;
}


static HRESULT UpdateOneFile(IInStream *inStream, 
    COutArchive &archive, 
    const CCompressionMethodMode &options,
    CAddCommon &compressor, 
    const CUpdateItem &updateItem, 
    UINT64 &currentComplexity,
    IArchiveUpdateCallback *updateCallback,
    CItemEx &fileHeader)
{
  CMyComPtr<IInStream> fileInStream;
  RINOK(updateCallback->GetStream(updateItem.IndexInClient, &fileInStream));

  bool isDirectory;
  UINT32 fileSize = updateItem.Size;
  fileHeader.UnPackSize = fileSize;

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
      CInStreamWithCRC *inStreamSpec = new CInStreamWithCRC;
      CMyComPtr<IInStream> inStream(inStreamSpec);
      inStreamSpec->Init(fileInStream);
      CCompressingResult compressingResult;
      CMyComPtr<IOutStream> outStream;
      archive.CreateStreamForCompressing(&outStream);

      CLocalProgress *localProgressSpec = new CLocalProgress;
      CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
      localProgressSpec->Init(updateCallback, true);
  
      CLocalCompressProgressInfo *localCompressProgressSpec = 
          new CLocalCompressProgressInfo;
      CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

      localCompressProgressSpec->Init(localProgress, &currentComplexity, NULL);

      RINOK(compressor.Compress(inStream, outStream, 
          fileSize, compressProgress, compressingResult));

      fileHeader.PackSize = (UINT32)compressingResult.PackSize;
      fileHeader.CompressionMethod = compressingResult.Method;
      fileHeader.ExtractVersion.Version = compressingResult.ExtractVersion;
      fileHeader.FileCRC = inStreamSpec->GetCRC();
      fileHeader.UnPackSize = (UINT32)inStreamSpec->GetSize();
    }
  }
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

static HRESULT Update2(COutArchive &archive, 
    IInStream *inStream,
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    const CCompressionMethodMode *options, 
    bool commentRangeAssigned,
    const CUpdateRange &commentRange,
    IArchiveUpdateCallback *updateCallback)
{
  UINT64 complexity = 0;
 
  int i;
  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &updateItem = updateItems[i];
    if (updateItem.NewData)
    {
      complexity += updateItem.Size;
      if (updateItem.Commented)
        complexity += updateItem.CommentRange.Size;
    }
    else
    {
      const CItemEx &inputItem = inputItems[updateItem.IndexInArchive];
      complexity += inputItem.GetLocalFullSize();
      complexity += inputItem.GetCentralExtraPlusCommentSize();
    }
  }

  complexity += updateItems.Size();
  complexity += updateItems.Size();

  if (complexity > _UI32_MAX)
    return E_NOTIMPL;

  if (commentRangeAssigned)
    complexity += commentRange.Size;

  complexity++; // end of central
  
  updateCallback->SetTotal(complexity);
  auto_ptr<CAddCommon> compressor;
  
  complexity = 0;
  
  CObjectVector<CItemEx> items;

  for(i = 0; i < updateItems.Size(); i++)
  {
    const CUpdateItem &updateItem = updateItems[i];
    RINOK(updateCallback->SetCompleted(&complexity));

    CItemEx item;
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
    complexity++;
  }
  DWORD centralDirStartPosition = archive.GetCurrentPosition();
  for(i = 0; i < items.Size(); i++)
  {
    archive.WriteCentralHeader(items[i]);
    const CUpdateItem &updateItem = updateItems[i];
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
      const CItemEx item = items[i];
      CUpdateRange range(item.CentralExtraPosition, 
        item.GetCentralExtraPlusCommentSize());
      RINOK(WriteRange(inStream, archive, range, 
          updateCallback, complexity));
      archive.MoveBasePosition(range.Size);
    }
    complexity++;
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

HRESULT Update(    
    const CObjectVector<CItemEx> &inputItems,
    const CObjectVector<CUpdateItem> &updateItems,
    IOutStream *outStream,
    CInArchive *inArchive,
    CCompressionMethodMode *compressionMethodMode,
    IArchiveUpdateCallback *updateCallback)
{
  DWORD startBlockSize;
  bool commentRangeAssigned;
  CUpdateRange commentRange;
  if(inArchive != 0)
  {
    CInArchiveInfo archiveInfo;
    inArchive->GetArchiveInfo(archiveInfo);
    startBlockSize = archiveInfo.StartPosition;
    commentRangeAssigned = archiveInfo.IsCommented();
    if (commentRangeAssigned)
    {
      commentRange.Position = archiveInfo.CommentPosition;
      commentRange.Size = archiveInfo.CommentSize;
    }
  }
  else
  {
    startBlockSize = 0;
    commentRangeAssigned = false;
  }
  
  COutArchive outArchive;
  outArchive.Create(outStream);
  if (startBlockSize > 0)
  {
    CMyComPtr<ISequentialInStream> inStream;
    inStream.Attach(inArchive->CreateLimitedStream(0, startBlockSize));
    RINOK(CopyBlockToArchive(inStream, outArchive, NULL));
    outArchive.MoveBasePosition(startBlockSize);
  }
  CMyComPtr<IInStream> inStream;
  if(inArchive != 0)
    inStream.Attach(inArchive->CreateStream());

  return Update2(outArchive, inStream, 
      inputItems, updateItems, 
      compressionMethodMode, 
      commentRangeAssigned, commentRange, updateCallback);
}

}}
