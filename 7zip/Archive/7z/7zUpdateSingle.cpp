// 7zUpdateSingle.cpp

#include "StdAfx.h"

#include "7zUpdateSingle.h"
#include "7zHandler.h"
#include "7zEncode.h"

#include "../../Compress/Copy/CopyCoder.h"
#include "../../Common/LimitedStreams.h"
#include "../../Common/ProgressUtils.h"
#include "../Common/ItemNameUtils.h"
#include "../Common/InStreamWithCRC.h"

static const kOneItemComplexity = 30;

namespace NArchive {
namespace N7z {

HRESULT CopyBlock(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, progress);
}

static HRESULT WriteRange(IInStream *inStream, 
    ISequentialOutStream *outStream, 
    const CUpdateRange &range, 
    IProgress *progress,
    UINT64 &currentComplexity)
{
  UINT64 position;
  inStream->Seek(range.Position, STREAM_SEEK_SET, &position);

  CLimitedSequentialInStream *streamSpec = new 
      CLimitedSequentialInStream;
  CMyComPtr<CLimitedSequentialInStream> inStreamLimited(streamSpec);
  streamSpec->Init(inStream, range.Size);


  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(progress, true);
  
  CLocalCompressProgressInfo *localCompressProgressSpec = 
      new CLocalCompressProgressInfo;
  CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

  localCompressProgressSpec->Init(localProgress, &currentComplexity, &currentComplexity);

  HRESULT result = CopyBlock(inStreamLimited, outStream, compressProgress);
  currentComplexity += range.Size;
  return result;
}

static int __cdecl CompareUpdateItems(const void *p1, const void *p2)
{
  const CUpdateItem &a1 = **(const CUpdateItem **)p1;
  const CUpdateItem &a2 = **(const CUpdateItem **)p2;
  
  int n;
  if (a1.IsDirectory != a2.IsDirectory)
  {
    if (a1.IsDirectory)
      return a1.IsAnti ? 1: -1;
    return a2.IsAnti ? -1: 1;
  }
  if (a1.IsDirectory)
  {
    if (a1.IsAnti != a2.IsAnti)
      return (a1.IsAnti ? 1 : -1);
    n = _wcsicmp(a1.Name, a2.Name);
    return (a1.IsAnti ? (-n) : n);
  }
  return _wcsicmp(a1.Name, a2.Name);
}

static void SetFileItemInfoFromUpdateInfo(const CUpdateItem &updateItem, 
    CFileItemInfo &fileHeaderInfo)
{
  fileHeaderInfo.Name = NItemName::MakeLegalName(updateItem.Name);
  fileHeaderInfo.IsDirectory = updateItem.IsDirectory;
  fileHeaderInfo.IsAnti = updateItem.IsAnti;
  if (updateItem.AttributesAreDefined)
    fileHeaderInfo.SetAttributes(updateItem.Attributes);
  // if (updateItem.CreationTimeIsDefined)
    // fileHeaderInfo.SetCreationTime(updateItem.CreationTime);
  if (updateItem.LastWriteTimeIsDefined)
    fileHeaderInfo.SetLastWriteTime(updateItem.LastWriteTime);
}


HRESULT UpdateOneFile(IInStream *inStream,
    const CCompressionMethodMode *options,                       
    COutArchive &archive,
    CEncoder &encoder,
    const CUpdateItem &updateItem, 
    UINT64 &currentComplexity,
    IArchiveUpdateCallback *updateCallback,
    CFileItemInfo &fileHeaderInfo,
    CFolderItemInfo &folderItem,
    CRecordVector<UINT64> &packSizes,
    bool &folderItemIsDefined)
{
  CMyComPtr<IInStream> fileInStream;

  RINOK(updateCallback->GetStream(updateItem.IndexInClient, &fileInStream));

  SetFileItemInfoFromUpdateInfo(updateItem, fileHeaderInfo);

  UINT64 size;
  UINT64 *sizePointer = NULL;
  UINT64 fileSize = updateItem.Size;
  if (fileInStream != NULL)
  {
    RINOK(fileInStream->Seek(0, STREAM_SEEK_END, &size));
    RINOK(fileInStream->Seek(0, STREAM_SEEK_SET, NULL));
    sizePointer = &size;
    fileSize = size;
  }
  fileHeaderInfo.UnPackSize = fileSize;

  if(updateItem.IsAnti || updateItem.IsDirectory || fileSize == 0)
    folderItemIsDefined = false;
  else
  {
    folderItemIsDefined = true;
    CInStreamWithCRC *inStreamSpec = new CInStreamWithCRC;
    CMyComPtr<IInStream> inStream(inStreamSpec);

    inStreamSpec->Init(fileInStream);
    
    CLocalProgress *localProgressSpec = new CLocalProgress;
    CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
    localProgressSpec->Init(updateCallback, true);
    CLocalCompressProgressInfo *localCompressProgressSpec = 
        new CLocalCompressProgressInfo;
    CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(localProgress, &currentComplexity, NULL);

    RINOK(encoder.Encode(inStream, sizePointer, 
        folderItem, archive.Stream, packSizes, compressProgress));

    fileHeaderInfo.FileCRC = inStreamSpec->GetCRC();
    fileHeaderInfo.FileCRCIsDefined = true;
    fileHeaderInfo.UnPackSize = inStreamSpec->GetSize();
  }
  // folderItem.NumFiles = 1;
  
  currentComplexity += fileSize;
  return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}


HRESULT UpdateArchiveStd(COutArchive &archive, 
    IInStream *inStream,
    const CCompressionMethodMode *method, 
    const CCompressionMethodMode *headerMethod,
    bool useAdditionalHeaderStreams, 
    bool compressMainHeader,
    const CArchiveDatabaseEx &database,
    CObjectVector<CUpdateItem> &updateItems,
    IArchiveUpdateCallback *updateCallback)
{
  RINOK(archive.SkeepPrefixArchiveHeader());
  UINT64 complexity = 0;

  // UINT32 compressIndex = 0, copyIndexIndex = 0;
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
      int fileIndex = updateItem.IndexInArchive;
      int folderIndex = database.FileIndexToFolderIndexMap[fileIndex];
      if (folderIndex >= 0)
        complexity += database.GetFolderFullPackSize(folderIndex);
    }
    complexity += kOneItemComplexity * 2;
  }
  updateCallback->SetTotal(complexity);
  CRecordVector<const CUpdateItem *> refItems;
  refItems.Reserve(updateItems.Size());
  for (i = 0; i < updateItems.Size(); i++)
  {
    CUpdateItem &updateItem = updateItems[i];
    if (!updateItem.NewProperties)
    {
      int fileIndex = updateItem.IndexInArchive;
      const CFileItemInfo &fileItem = database.Files[fileIndex];
      updateItem.IsAnti = fileItem.IsAnti;
      updateItem.IsDirectory = fileItem.IsDirectory;
      updateItem.Name = fileItem.Name;
    }
    refItems.Add(&updateItems[i]);
  }
  
  qsort(&refItems.Front(), refItems.Size(), sizeof(refItems[0]), CompareUpdateItems);

  CArchiveDatabase newDatabase;
  {
    complexity = 0;
    
    std::auto_ptr<CEncoder> encoder;
    for(i = 0; i < updateItems.Size(); i++)
    {
      const CUpdateItem &updateItemInfo = *refItems[i];

      CFileItemInfo fileItem;
      CFolderItemInfo folderItem;
      RINOK(updateCallback->SetCompleted(&complexity));
      if (updateItemInfo.NewData)
      {
        if (encoder.get() == 0)
          encoder = (std::auto_ptr<CEncoder>)(new CEncoder(method));
        // mixerCoderSpec->SetCoderInfo(0, NULL, NULL, progress);
        bool folderItemIsDefined;
        RINOK(UpdateOneFile(inStream, method, 
          archive, *encoder, 
          updateItemInfo, 
          complexity,  updateCallback, fileItem, folderItem, 
          newDatabase.PackSizes, folderItemIsDefined));
        if (folderItemIsDefined)
          newDatabase.Folders.Add(folderItem);
      }
      else
      {
        int fileIndex = updateItemInfo.IndexInArchive;
        int folderIndex = database.FileIndexToFolderIndexMap[fileIndex];
        fileItem = database.Files[fileIndex];
        if (updateItemInfo.NewProperties)
          SetFileItemInfoFromUpdateInfo(updateItemInfo, fileItem);
        if (folderIndex >= 0)
        {
          CUpdateRange range(database.GetFolderStreamPos(folderIndex, 0),
            database.GetFolderFullPackSize(folderIndex));
          RINOK(WriteRange(inStream, archive.Stream, range, 
            updateCallback, complexity));
          const CFolderItemInfo &folder = database.Folders[folderIndex];
          UINT64 packStreamIndex = database.FolderStartPackStreamIndex[folderIndex];
          for (int j = 0; j < folder.PackStreams.Size(); j++)
            newDatabase.PackSizes.Add(database.PackSizes[packStreamIndex + j]);
          newDatabase.Folders.Add(folder);
        }
      }
      newDatabase.Files.Add(fileItem);
      complexity += kOneItemComplexity;
    }
    
    newDatabase.NumUnPackStreamsVector.Reserve(newDatabase.Folders.Size());
    for (i = 0; i < newDatabase.Folders.Size(); i++)
      newDatabase.NumUnPackStreamsVector.Add(1);
  }

  return archive.WriteDatabase(newDatabase, headerMethod, 
      useAdditionalHeaderStreams, compressMainHeader);
}

}}