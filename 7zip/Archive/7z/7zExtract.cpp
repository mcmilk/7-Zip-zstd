// 7zExtract.cpp

#include "StdAfx.h"

#include "7zHandler.h"
#include "7zFolderOutStream.h"
#include "7zMethods.h"
#include "7zDecode.h"
// #include "7z1Decode.h"

#include "../../../Common/ComTry.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"

namespace NArchive {
namespace N7z {

struct CExtractFolderInfo
{
  #ifdef _7Z_VOL
  int VolumeIndex;
  #endif
  int FileIndex;
  int FolderIndex;
  CBoolVector ExtractStatuses;
  UInt64 UnPackSize;
  CExtractFolderInfo(
    #ifdef _7Z_VOL
    int volumeIndex, 
    #endif
    int fileIndex, int folderIndex): 
    #ifdef _7Z_VOL
    VolumeIndex(volumeIndex),
    #endif
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

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 testModeSpec, IArchiveExtractCallback *extractCallbackSpec)
{
  COM_TRY_BEGIN
  bool testMode = (testModeSpec != 0);
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  UInt64 importantTotalUnPacked = 0;
  UInt64 censoredTotalUnPacked = 0, censoredTotalPacked = 0;

  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = 
    #ifdef _7Z_VOL
    _refs.Size();
    #else
    _database.Files.Size();
    #endif

  if(numItems == 0)
    return S_OK;

  /*
  if(_volumes.Size() != 1)
    return E_FAIL;
  const CVolume &volume = _volumes.Front();
  const CArchiveDatabaseEx &_database = volume.Database;
  IInStream *_inStream = volume.Stream;
  */
  
  CObjectVector<CExtractFolderInfo> extractFolderInfoVector;
  for(UInt32 ii = 0; ii < numItems; ii++)
  {
    // int fileIndex = allFilesMode ? indexIndex : indices[indexIndex];
    int ref2Index = allFilesMode ? ii : indices[ii];
    // const CRef2 &ref2 = _refs[ref2Index];

    // for(UInt32 ri = 0; ri < ref2.Refs.Size(); ri++)
    {
      #ifdef _7Z_VOL
      // const CRef &ref = ref2.Refs[ri];
      const CRef &ref = _refs[ref2Index];

      int volumeIndex = ref.VolumeIndex;
      const CVolume &volume = _volumes[volumeIndex];
      const CArchiveDatabaseEx &database = volume.Database;
      int fileIndex = ref.ItemIndex;
      #else
      const CArchiveDatabaseEx &database = _database;
      int fileIndex = ref2Index;
      #endif

      int folderIndex = database.FileIndexToFolderIndexMap[fileIndex];
      if (folderIndex < 0)
      {
        extractFolderInfoVector.Add(CExtractFolderInfo(
            #ifdef _7Z_VOL
            volumeIndex, 
            #endif
            fileIndex, -1));
        continue;
      }
      if (extractFolderInfoVector.IsEmpty() || 
        folderIndex != extractFolderInfoVector.Back().FolderIndex 
        #ifdef _7Z_VOL
        || volumeIndex != extractFolderInfoVector.Back().VolumeIndex
        #endif
        )
      {
        extractFolderInfoVector.Add(CExtractFolderInfo(
            #ifdef _7Z_VOL
            volumeIndex, 
            #endif
            -1, folderIndex));
        const CFolder &folderInfo = database.Folders[folderIndex];
        UInt64 unPackSize = folderInfo.GetUnPackSize();
        importantTotalUnPacked += unPackSize;
        extractFolderInfoVector.Back().UnPackSize = unPackSize;
      }
      
      CExtractFolderInfo &efi = extractFolderInfoVector.Back();
      
      // const CFolderInfo &folderInfo = m_dam_Folders[folderIndex];
      UInt32 startIndex = (UInt32)database.FolderStartFileIndex[folderIndex];
      for (UInt32 index = efi.ExtractStatuses.Size();
          index <= fileIndex - startIndex; index++)
      {
        // UInt64 unPackSize = _database.Files[startIndex + index].UnPackSize;
        // Count partial_folder_size
        // efi.UnPackSize += unPackSize;
        // importantTotalUnPacked += unPackSize;
        efi.ExtractStatuses.Add(index == fileIndex - startIndex);
      }
    }
  }

  extractCallback->SetTotal(importantTotalUnPacked);

  CDecoder decoder(true);
  // CDecoder1 decoder;

  UInt64 currentImportantTotalUnPacked = 0;
  UInt64 totalFolderUnPacked;

  for(int i = 0; i < extractFolderInfoVector.Size(); i++, 
      currentImportantTotalUnPacked += totalFolderUnPacked)
  {
    const CExtractFolderInfo &efi = extractFolderInfoVector[i];
    totalFolderUnPacked = efi.UnPackSize;

    RINOK(extractCallback->SetCompleted(&currentImportantTotalUnPacked));

    CFolderOutStream *folderOutStream = new CFolderOutStream;
    CMyComPtr<ISequentialOutStream> outStream(folderOutStream);

    #ifdef _7Z_VOL
    const CVolume &volume = _volumes[efi.VolumeIndex];
    const CArchiveDatabaseEx &database = volume.Database;
    #else
    const CArchiveDatabaseEx &database = _database;
    #endif

    UInt32 startIndex;
    if (efi.FileIndex >= 0)
      startIndex = efi.FileIndex;
    else
      startIndex = (UInt32)database.FolderStartFileIndex[efi.FolderIndex];


    HRESULT result = folderOutStream->Init(&database, 
        #ifdef _7Z_VOL
        volume.StartRef2Index, 
        #else
        0,
        #endif
        startIndex, 
        &efi.ExtractStatuses, extractCallback, testMode);

    RINOK(result);

    if (efi.FileIndex >= 0)
      continue;

    UInt32 folderIndex = efi.FolderIndex;
    const CFolder &folderInfo = database.Folders[folderIndex];

    CLocalProgress *localProgressSpec = new CLocalProgress;
    CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
    localProgressSpec->Init(extractCallback, false);

    CLocalCompressProgressInfo *localCompressProgressSpec = 
        new CLocalCompressProgressInfo;
    CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(progress, NULL, &currentImportantTotalUnPacked);

    UInt32 packStreamIndex = database.FolderStartPackStreamIndex[folderIndex];
    UInt64 folderStartPackPos = database.GetFolderStreamPos(folderIndex, 0);

    #ifndef _NO_CRYPTO
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    if (extractCallback)
      extractCallback.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
    #endif

    try
    {
      HRESULT result = decoder.Decode(
          #ifdef _7Z_VOL
          volume.Stream,
          #else
          _inStream,
          #endif
          folderStartPackPos, 
          &database.PackSizes[packStreamIndex],
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
