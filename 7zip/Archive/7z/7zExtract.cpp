// 7zExtract.cpp

#include "StdAfx.h"

#include "7zHandler.h"
#include "7zFolderOutStream.h"
#include "7zMethods.h"
#include "7zDecode.h"

#include "../../../Common/ComTry.h"
#include "../../Common/MultiStream.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"

namespace NArchive {
namespace N7z {

struct CExtractFolderInfo
{
  int FileIndex;
  int FolderIndex;
  CBoolVector ExtractStatuses;
  UINT64 UnPackSize;
  CExtractFolderInfo(int fileIndex, int folderIndex): 
    FileIndex(fileIndex),
    FolderIndex(folderIndex), 
    UnPackSize(0) 
  {
    if (fileIndex >= 0)
    {
      ExtractStatuses.Reserve(1);
      ExtractStatuses.Add(true);
    }
  };
};

STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 testModeSpec, IArchiveExtractCallback *extractCallbackSpec)
{
  COM_TRY_BEGIN
  bool testMode = (testModeSpec != 0);
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  UINT64 importantTotalUnPacked = 0;
  UINT64 censoredTotalUnPacked = 0, censoredTotalPacked = 0;

  if (indices == 0)
    numItems = _database.Files.Size();

  if(numItems == 0)
    return S_OK;

  CObjectVector<CExtractFolderInfo> extractFolderInfoVector;
  for(UINT32 indexIndex = 0; indexIndex < numItems; indexIndex++)
  {
    int fileIndex = (indices != 0) ? indices[indexIndex] : indexIndex;
    int folderIndex = _database.FileIndexToFolderIndexMap[fileIndex];
    if (folderIndex < 0)
    {
      extractFolderInfoVector.Add(CExtractFolderInfo(fileIndex, -1));
      continue;
    }
    if (extractFolderInfoVector.IsEmpty() || 
        folderIndex != extractFolderInfoVector.Back().FolderIndex)
    {
      extractFolderInfoVector.Add(CExtractFolderInfo(-1, folderIndex));
      const CFolderItemInfo &folderInfo = _database.Folders[folderIndex];
      // Count full_folder_size
      UINT64 unPackSize = folderInfo.GetUnPackSize();
      importantTotalUnPacked += unPackSize;
      extractFolderInfoVector.Back().UnPackSize = unPackSize;
    }

    CExtractFolderInfo &extractFolderInfo = extractFolderInfoVector.Back();

    // const CFolderInfo &folderInfo = m_dam_Folders[folderIndex];
    UINT32 startIndex = (UINT32)_database.FolderStartFileIndex[folderIndex];
    for (UINT32 index = extractFolderInfo.ExtractStatuses.Size();
        index <= fileIndex - startIndex; index++)
    {
      UINT64 unPackSize = _database.Files[startIndex + index].UnPackSize;
      // Count partial_folder_size
      // extractFolderInfo.UnPackSize += unPackSize;
      // importantTotalUnPacked += unPackSize;
      extractFolderInfo.ExtractStatuses.Add(index == fileIndex - startIndex);
    }
  }

  extractCallback->SetTotal(importantTotalUnPacked);

  CDecoder decoder;

  UINT64 currentImportantTotalUnPacked = 0;
  UINT64 totalFolderUnPacked;

  for(int i = 0; i < extractFolderInfoVector.Size(); i++, 
      currentImportantTotalUnPacked += totalFolderUnPacked)
  {
    CExtractFolderInfo &extractFolderInfo = extractFolderInfoVector[i];
    totalFolderUnPacked = extractFolderInfo.UnPackSize;

    RINOK(extractCallback->SetCompleted(&currentImportantTotalUnPacked));

    CFolderOutStream *folderOutStream = new CFolderOutStream;
    CMyComPtr<ISequentialOutStream> outStream(folderOutStream);

    UINT32 startIndex;
    if (extractFolderInfo.FileIndex >= 0)
      startIndex = extractFolderInfo.FileIndex;
    else
      startIndex = (UINT32)_database.FolderStartFileIndex[extractFolderInfo.FolderIndex];


    RINOK(folderOutStream->Init(&_database, startIndex, 
        &extractFolderInfo.ExtractStatuses, extractCallback, testMode));

    if (extractFolderInfo.FileIndex >= 0)
      continue;

    UINT32 folderIndex = extractFolderInfo.FolderIndex;
    const CFolderItemInfo &folderInfo = _database.Folders[folderIndex];

    CObjectVector< CMyComPtr<ISequentialInStream> > inStreams;

    CLockedInStream lockedInStream;
    lockedInStream.Init(_inStream);


    UINT64 folderStartPackStreamIndex = _database.FolderStartPackStreamIndex[folderIndex];

    for (int j = 0; j < folderInfo.PackStreams.Size(); j++)
    {
      const CPackStreamInfo &packStreamInfo = folderInfo.PackStreams[j];
      CLockedSequentialInStreamImp *lockedStreamImpSpec = new 
          CLockedSequentialInStreamImp;
      CMyComPtr<ISequentialInStream> lockedStreamImp = lockedStreamImpSpec;
      UINT64 streamStartPos = _database.GetFolderStreamPos(folderIndex, j);
      lockedStreamImpSpec->Init(&lockedInStream, streamStartPos);

      CLimitedSequentialInStream *streamSpec = new 
          CLimitedSequentialInStream;
      CMyComPtr<ISequentialInStream> inStream = streamSpec;
      streamSpec->Init(lockedStreamImp, 
          _database.PackSizes[(UINT32)folderStartPackStreamIndex + j]);
      inStreams.Add(inStream);
    }

    CLocalProgress *localProgressSpec = new CLocalProgress;
    CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
    localProgressSpec->Init(extractCallback, false);

    CLocalCompressProgressInfo *localCompressProgressSpec = 
        new CLocalCompressProgressInfo;
    CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(progress, NULL, &currentImportantTotalUnPacked);

    UINT32 packStreamIndex = _database.FolderStartPackStreamIndex[folderIndex];
    UINT64 folderStartPackPos = _database.GetFolderStreamPos(folderIndex, 0);

    #ifndef _NO_CRYPTO
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    if (extractCallback)
      extractCallback.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
    #endif

    try
    {
      HRESULT result = decoder.Decode(_inStream,
          folderStartPackPos, 
          &_database.PackSizes[packStreamIndex],
          folderInfo,
          outStream,
          compressProgress
          #ifndef _NO_CRYPTO
          , getTextPassword
          #endif
          );

      if (result == S_FALSE)
      {
        RINOK(folderOutStream->FlushCorrupted(NArchive::NExtract::NOperationResult::kDataError));
        continue;
      }
      if (result == E_NOTIMPL)
      {
        RINOK(folderOutStream->FlushCorrupted(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
      }
      if (result != S_OK)
        return result;
      RINOK(folderOutStream->WasWritingFinished());
    }
    catch(...)
    {
      RINOK(folderOutStream->FlushCorrupted(NArchive::NExtract::NOperationResult::kDataError));
      continue;
    }
  }
  return S_OK;
  COM_TRY_END
}

}}