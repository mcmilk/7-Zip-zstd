// UpdateArchiveEngine.cpp

#include "StdAfx.h"

#include "UpdateEngine.h"

#include "../../../Compress/Interface/CompressInterface.h"

#include "Compression/CopyCoder.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"

#include "Windows/Defs.h"
#include "Windows/COM.h"

#include "../Common/InStreamWithCRC.h"

#include "Archive/Common/ItemNameUtils.h"
#include "Handler.h"

#include "Encode.h"

using namespace NArchive;
using namespace N7z;
using namespace std;

static const kOneItemComplexity = 30;

namespace NArchive {
namespace N7z {

HRESULT CopyBlock(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> copyCoder = copyCoderSpec;

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

  HRESULT result = CopyBlock(inStreamLimited, outStream, compressProgress);
  currentComplexity += range.Size;
  return result;
}

struct CRefItem2
{
  bool IsCompressItem;
  bool IsAnti;
  bool IsDirectory;
  UINT32 CompressIndex;
  UINT32 CopyIndex;
  const wchar_t *Name;
};

static int CompareUpdateItems(const void *p1, const void *p2)
{
  const CRefItem2 &a1 = *((CRefItem2 *)p1);
  const CRefItem2 &a2 = *((CRefItem2 *)p2);
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


HRESULT UpdateOneFile(IInStream *inStream,
    const CCompressionMethodMode *options,                       
    COutArchive &archive,
    CEncoder &encoder,
    const CUpdateItemInfo &updateItem, 
    UINT64 &currentComplexity,
    IUpdateCallBack *updateCallback,
    CFileItemInfo &fileHeaderInfo,
    CFolderItemInfo &folderItem,
    CRecordVector<UINT64> &packSizes,
    bool &folderItemIsDefined)
{
  CComPtr<IInStream> fileInStream;

  RETURN_IF_NOT_S_OK(updateCallback->CompressOperation(
      updateItem.IndexInClient, &fileInStream));

  UINT64 size;
  UINT64 *sizePointer = NULL;
  if (fileInStream != NULL)
  {
    CComPtr<IInStream> fullInStream;
    if (fileInStream.QueryInterface(&fullInStream) == S_OK)
    {
      RETURN_IF_NOT_S_OK(fullInStream->Seek(0, STREAM_SEEK_END, &size));
      RETURN_IF_NOT_S_OK(fullInStream->Seek(0, STREAM_SEEK_SET, NULL));
      sizePointer = &size;
    }
  }

  UINT64 fileSize = updateItem.Size;
  
  // ConvertUnicodeToUTF(NItemName::MakeLegalName(updateItem.Name), fileHeaderInfo.Name); // test it
  fileHeaderInfo.Name = NItemName::MakeLegalName(updateItem.Name);

  fileHeaderInfo.IsDirectory = updateItem.IsDirectory;
  fileHeaderInfo.IsAnti = updateItem.IsAnti;

  if(updateItem.IsAnti || updateItem.IsDirectory || fileSize == 0)
    folderItemIsDefined = false;
  else
  {
    folderItemIsDefined = true;
    CComObjectNoLock<CInStreamWithCRC> *inStreamSpec = 
      new CComObjectNoLock<CInStreamWithCRC>;
    CComPtr<IInStream> inStream(inStreamSpec);

    inStreamSpec->Init(fileInStream);
    
    CComObjectNoLock<CLocalProgress> *localProgressSpec = 
      new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
    localProgressSpec->Init(updateCallback, true);
    CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(localProgress, &currentComplexity, NULL);

    RETURN_IF_NOT_S_OK(encoder.Encode(inStream, sizePointer, 
        folderItem,
        archive.m_Stream,
        packSizes,
        compressProgress));

    fileHeaderInfo.FileCRC = inStreamSpec->GetCRC();
    fileHeaderInfo.FileCRCIsDefined = true;

  }
  fileHeaderInfo.UnPackSize = fileSize;
  // folderItem.NumFiles = 1;
  
  if (updateItem.AttributesAreDefined)
    fileHeaderInfo.SetAttributes(updateItem.Attributes);

  // if (updateItem.CreationTimeIsDefined)
    // fileHeaderInfo.SetCreationTime(updateItem.CreationTime);
  
  if (updateItem.LastWriteTimeIsDefined)
    fileHeaderInfo.SetLastWriteTime(updateItem.LastWriteTime);

  currentComplexity += fileSize;
  return updateCallback->OperationResult(NArchiveHandler::NUpdate::NOperationResult::kOK);
}


HRESULT UpdateArchiveStd(COutArchive &archive, 
    IInStream *inStream,
    const CCompressionMethodMode *method, 
    const CCompressionMethodMode *headerMethod,
    const NArchive::N7z::CArchiveDatabaseEx &database,
    const CRecordVector<bool> &compressStatuses,
    const CObjectVector<CUpdateItemInfo> &updateItems,
    const CRecordVector<UINT32> &copyIndices,    
    IUpdateCallBack *updateCallback)
{
  RETURN_IF_NOT_S_OK(archive.SkeepPrefixArchiveHeader());
  UINT64 complexity = 0;

  UINT32 compressIndex = 0, copyIndexIndex = 0;
  int i;
  for(i = 0; i < compressStatuses.Size(); i++)
  {
    if (compressStatuses[i])
    {
      const CUpdateItemInfo &updateItem = updateItems[compressIndex++];
      complexity += updateItem.Size;
      if (updateItem.Commented)
        complexity += updateItem.CommentRange.Size;
    }
    else
    {
      int fileIndex = copyIndices[copyIndexIndex++];
      // const CFileItemInfoEx &anInputItem = database.Files[copyIndices[copyIndexIndex++]];
      int folderIndex = database.FileIndexToFolderIndexMap[fileIndex];
      if (folderIndex >= 0)
        complexity += database.GetFolderFullPackSize(folderIndex);
    }
    // complexity += kOneItemComplexity * 3;
    complexity += kOneItemComplexity * 2;
  }
  
  updateCallback->SetTotal(complexity);

  compressIndex = copyIndexIndex = 0;

  CRecordVector<CRefItem2> refItems;
  refItems.Reserve(compressStatuses.Size());
  for (i = 0; i < compressStatuses.Size(); i++)
  {
    CRefItem2 refItem;
    if (compressStatuses[i])
    {
      const CUpdateItemInfo &updateItemInfo = updateItems[compressIndex];
      refItem.IsCompressItem = true;
      refItem.CompressIndex = compressIndex;
      refItem.IsAnti = updateItemInfo.IsAnti;
      refItem.IsDirectory = updateItemInfo.IsDirectory;
      refItem.Name = updateItemInfo.Name;
      compressIndex++;
    }
    else
    {
      int fileIndex = copyIndices[copyIndexIndex];
      const CFileItemInfo &fileItem = database.Files[fileIndex];
      refItem.CopyIndex = fileIndex;
      refItem.IsCompressItem = false;
      refItem.IsAnti = false;
      refItem.IsDirectory = fileItem.IsDirectory;
      refItem.Name = fileItem.Name;
      copyIndexIndex++;
    }
    refItems.Add(refItem);
  }
  
  qsort(&refItems.Front(), refItems.Size(), sizeof(refItems[0]), CompareUpdateItems);

  CArchiveDatabase newDatabase;
  {
    complexity = 0;
    
    std::auto_ptr<CEncoder> encoder;
    for(i = 0; i < compressStatuses.Size(); i++)
    {
      const CRefItem2 &refItem = refItems[i];
      CFileItemInfo fileItem;
      CFolderItemInfo folderItem;
      RETURN_IF_NOT_S_OK(updateCallback->SetCompleted(&complexity));
      if (refItem.IsCompressItem)
      {
        if (encoder.get() == 0)
          encoder = (auto_ptr<CEncoder>)(new CEncoder(method));
        // mixerCoderSpec->SetCoderInfo(0, NULL, NULL, progress);
        const CUpdateItemInfo &updateItemInfo = updateItems[refItem.CompressIndex];
        bool folderItemIsDefined;
        RETURN_IF_NOT_S_OK(UpdateOneFile(inStream, method, 
          archive, *encoder, 
          updateItemInfo, 
          complexity,  updateCallback, fileItem, folderItem, 
          newDatabase.PackSizes, folderItemIsDefined));
        if (folderItemIsDefined)
          newDatabase.Folders.Add(folderItem);
      }
      else
      {
        int fileIndex = refItem.CopyIndex;
        int folderIndex = database.FileIndexToFolderIndexMap[fileIndex];
        fileItem = database.Files[fileIndex];
        if (folderIndex >= 0)
        {
          CUpdateRange range(database.GetFolderStreamPos(folderIndex, 0),
            database.GetFolderFullPackSize(folderIndex));
          RETURN_IF_NOT_S_OK(WriteRange(inStream, archive.m_Stream, range, 
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

  return archive.WriteDatabase(newDatabase, headerMethod);
}
}}