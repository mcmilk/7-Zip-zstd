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

#include "ItemNameUtils.h"
#include "Handler.h"

#include "Encode.h"

using namespace NArchive;
using namespace N7z;
using namespace std;

static const kOneItemComplexity = 30;

namespace NArchive {
namespace N7z {

HRESULT CopyBlock(ISequentialInStream *anInStream, 
    ISequentialOutStream *anOutStream, ICompressProgressInfo *aProgress)
{
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> aCopyCoder = aCopyCoderSpec;

  return aCopyCoder->Code(anInStream, anOutStream, NULL, NULL, aProgress);
}

static HRESULT WriteRange(IInStream *anInStream, 
    ISequentialOutStream *anOutStream, 
    const CUpdateRange &aRange, 
    IProgress *aProgress,
    UINT64 &aCurrentComplexity)
{
  UINT64 aPosition;
  anInStream->Seek(aRange.Position, STREAM_SEEK_SET, &aPosition);

  CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
      CComObjectNoLock<CLimitedSequentialInStream>;
  CComPtr<CLimitedSequentialInStream> aInStreamLimited(aStreamSpec);
  aStreamSpec->Init(anInStream, aRange.Size);


  CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> aLocalProgress = aLocalProgressSpec;
  aLocalProgressSpec->Init(aProgress, true);
  
  CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
  CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;

  aLocalCompressProgressSpec->Init(aLocalProgress, &aCurrentComplexity, &aCurrentComplexity);

  HRESULT aResult = CopyBlock(aInStreamLimited, anOutStream, aCompressProgress);
  aCurrentComplexity += aRange.Size;
  return aResult;
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


HRESULT UpdateOneFile(IInStream *anInStream,
    const CCompressionMethodMode *anOptions,                       
    COutArchive &anArchive,
    CEncoder &anEncoder,
    const CUpdateItemInfo &anUpdateItem, 
    UINT64 &aCurrentComplexity,
    IUpdateCallBack *anUpdateCallBack,
    CFileItemInfo &aFileHeaderInfo,
    CFolderItemInfo &aFolderItem,
    CRecordVector<UINT64> &aPackSizes,
    bool &aFolderItemIsDefined)
{
  CComPtr<IInStream> aFileInStream;

  RETURN_IF_NOT_S_OK(anUpdateCallBack->CompressOperation(
      anUpdateItem.IndexInClient, &aFileInStream));

  UINT64 aSize;
  UINT64 *aSizePointer = NULL;
  if (aFileInStream != NULL)
  {
    CComPtr<IInStream> aFullInStream;
    if (aFileInStream.QueryInterface(&aFullInStream) == S_OK)
    {
      RETURN_IF_NOT_S_OK(aFullInStream->Seek(0, STREAM_SEEK_END, &aSize));
      RETURN_IF_NOT_S_OK(aFullInStream->Seek(0, STREAM_SEEK_SET, NULL));
      aSizePointer = &aSize;
    }
  }

  UINT64 aFileSize = anUpdateItem.Size;
  
  // ConvertUnicodeToUTF(NItemName::MakeLegalName(anUpdateItem.Name), aFileHeaderInfo.Name); // test it
  aFileHeaderInfo.Name = NItemName::MakeLegalName(anUpdateItem.Name);

  aFileHeaderInfo.IsDirectory = anUpdateItem.IsDirectory;
  aFileHeaderInfo.IsAnti = anUpdateItem.IsAnti;

  if(anUpdateItem.IsAnti || anUpdateItem.IsDirectory || aFileSize == 0)
    aFolderItemIsDefined = false;
  else
  {
    aFolderItemIsDefined = true;
    CComObjectNoLock<CInStreamWithCRC> *anInStreamSpec = 
      new CComObjectNoLock<CInStreamWithCRC>;
    CComPtr<IInStream> anInStream(anInStreamSpec);

    anInStreamSpec->Init(aFileInStream);
    
    CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = 
      new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> aLocalProgress = aLocalProgressSpec;
    aLocalProgressSpec->Init(anUpdateCallBack, true);
    CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
    aLocalCompressProgressSpec->Init(aLocalProgress, &aCurrentComplexity, NULL);

    RETURN_IF_NOT_S_OK(anEncoder.Encode(anInStream, aSizePointer, 
        aFolderItem,
        anArchive.m_Stream,
        aPackSizes,
        aCompressProgress));

    aFileHeaderInfo.FileCRC = anInStreamSpec->GetCRC();
    aFileHeaderInfo.FileCRCIsDefined = true;

  }
  aFileHeaderInfo.UnPackSize = aFileSize;
  // aFolderItem.NumFiles = 1;
  
  if (anUpdateItem.AttributesAreDefined)
    aFileHeaderInfo.SetAttributes(anUpdateItem.Attributes);

  // if (anUpdateItem.CreationTimeIsDefined)
    // aFileHeaderInfo.SetCreationTime(anUpdateItem.CreationTime);
  
  if (anUpdateItem.LastWriteTimeIsDefined)
    aFileHeaderInfo.SetLastWriteTime(anUpdateItem.LastWriteTime);

  aCurrentComplexity += aFileSize;
  return anUpdateCallBack->OperationResult(NArchiveHandler::NUpdate::NOperationResult::kOK);
}


HRESULT UpdateArchiveStd(COutArchive &anArchive, 
    IInStream *anInStream,
    const CCompressionMethodMode *aMethod, 
    const CCompressionMethodMode *aHeaderMethod,
    const NArchive::N7z::CArchiveDatabaseEx &aDatabase,
    const CRecordVector<bool> &aCompressStatuses,
    const CObjectVector<CUpdateItemInfo> &anUpdateItems,
    const CRecordVector<UINT32> &aCopyIndexes,    
    IUpdateCallBack *anUpdateCallBack)
{
  RETURN_IF_NOT_S_OK(anArchive.SkeepPrefixArchiveHeader());
  UINT64 aComplexity = 0;

  UINT32 aCompressIndex = 0, aCopyIndexIndex = 0;
  for(int i = 0; i < aCompressStatuses.Size(); i++)
  {
    if (aCompressStatuses[i])
    {
      const CUpdateItemInfo &anUpdateItem = anUpdateItems[aCompressIndex++];
      aComplexity += anUpdateItem.Size;
      if (anUpdateItem.Commented)
        aComplexity += anUpdateItem.CommentRange.Size;
    }
    else
    {
      int aFileIndex = aCopyIndexes[aCopyIndexIndex++];
      // const CFileItemInfoEx &anInputItem = aDatabase.m_Files[aCopyIndexes[aCopyIndexIndex++]];
      int aFolderIndex = aDatabase.m_FileIndexToFolderIndexMap[aFileIndex];
      if (aFolderIndex >= 0)
        aComplexity += aDatabase.GetFolderFullPackSize(aFolderIndex);
    }
    // aComplexity += kOneItemComplexity * 3;
    aComplexity += kOneItemComplexity * 2;
  }
  
  anUpdateCallBack->SetTotal(aComplexity);

  aCompressIndex = aCopyIndexIndex = 0;

  int aNumFiles = anUpdateItems.Size();
  CRecordVector<CRefItem2> aRefItems;
  aRefItems.Reserve(aNumFiles);
  for (i = 0; i < aNumFiles; i++)
  {
    CRefItem2 aRefItem;
    if (aCompressStatuses[i])
    {
      const CUpdateItemInfo &anUpdateItemInfo = anUpdateItems[aCompressIndex];
      aRefItem.IsCompressItem = true;
      aRefItem.CompressIndex = aCompressIndex;
      aRefItem.IsAnti = anUpdateItemInfo.IsAnti;
      aRefItem.IsDirectory = anUpdateItemInfo.IsDirectory;
      aRefItem.Name = anUpdateItemInfo.Name;
      aCompressIndex++;
    }
    else
    {
      int aFileIndex = aCopyIndexes[aCopyIndexIndex];
      const CFileItemInfo &aFileItem = aDatabase.m_Files[aFileIndex];
      aRefItem.CopyIndex = aFileIndex;
      aRefItem.IsCompressItem = false;
      aRefItem.IsAnti = false;
      aRefItem.IsDirectory = aFileItem.IsDirectory;
      aRefItem.Name = aFileItem.Name;
      aCopyIndexIndex++;
    }
    aRefItems.Add(aRefItem);
  }
  
  qsort(&aRefItems.Front(), aRefItems.Size(), sizeof(aRefItems[0]), CompareUpdateItems);

  CArchiveDatabase aNewDatabase;
  {
    aComplexity = 0;
    
    std::auto_ptr<CEncoder> anEncoder;
    for(i = 0; i < aCompressStatuses.Size(); i++)
    {
      const CRefItem2 &aRefItem = aRefItems[i];
      CFileItemInfo aFileItem;
      CFolderItemInfo aFolderItem;
      RETURN_IF_NOT_S_OK(anUpdateCallBack->SetCompleted(&aComplexity));
      if (aRefItem.IsCompressItem)
      {
        if (anEncoder.get() == 0)
          anEncoder = (auto_ptr<CEncoder>)(new CEncoder(aMethod));
        // aMixerCoderSpec->SetCoderInfo(0, NULL, NULL, aProgress);
        const CUpdateItemInfo &anUpdateItemInfo = anUpdateItems[aRefItem.CompressIndex];
        bool aFolderItemIsDefined;
        RETURN_IF_NOT_S_OK(UpdateOneFile(anInStream, aMethod, 
          anArchive, *anEncoder, 
          anUpdateItemInfo, 
          aComplexity,  anUpdateCallBack, aFileItem, aFolderItem, 
          aNewDatabase.m_PackSizes, aFolderItemIsDefined));
        if (aFolderItemIsDefined)
          aNewDatabase.m_Folders.Add(aFolderItem);
      }
      else
      {
        int aFileIndex = aRefItem.CopyIndex;
        int aFolderIndex = aDatabase.m_FileIndexToFolderIndexMap[aFileIndex];
        aFileItem = aDatabase.m_Files[aFileIndex];
        if (aFolderIndex >= 0)
        {
          CUpdateRange aRange(aDatabase.GetFolderStreamPos(aFolderIndex, 0),
            aDatabase.GetFolderFullPackSize(aFolderIndex));
          RETURN_IF_NOT_S_OK(WriteRange(anInStream, anArchive.m_Stream, aRange, 
            anUpdateCallBack, aComplexity));
          const CFolderItemInfo &aFolder = aDatabase.m_Folders[aFolderIndex];
          UINT64 aPackStreamIndex = aDatabase.m_FolderStartPackStreamIndex[aFolderIndex];
          for (int j = 0; j < aFolder.PackStreams.Size(); j++)
            aNewDatabase.m_PackSizes.Add(aDatabase.m_PackSizes[aPackStreamIndex + j]);
          aNewDatabase.m_Folders.Add(aFolder);
        }
      }
      aNewDatabase.m_Files.Add(aFileItem);
      aComplexity += kOneItemComplexity;
    }
    
    aNewDatabase.m_NumUnPackStreamsVector.Reserve(aNewDatabase.m_Folders.Size());
    for (i = 0; i < aNewDatabase.m_Folders.Size(); i++)
      aNewDatabase.m_NumUnPackStreamsVector.Add(1);
  }

  return anArchive.WriteDatabase(aNewDatabase, aHeaderMethod);
}
}}