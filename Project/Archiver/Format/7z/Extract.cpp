// Extract.cpp

#include "StdAfx.h"

#include "Handler.h"
#include "Util/MultiStream.h"
#include "FolderOutStream.h"

#include "RegistryInfo.h"

#include "Interface/StreamObjects.h"
#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"

#include "Decode.h"

#include "Windows/COMTry.h"

using namespace NArchive;
using namespace N7z;

struct CExtractFolderInfo
{
  int FileIndex;
  int FolderIndex;
  CBoolVector ExtractStatuses;
  UINT64 UnPackSize;
  CExtractFolderInfo(int aFileIndex, int aFolderIndex): 
    FileIndex(aFileIndex),
    FolderIndex(aFolderIndex), 
    UnPackSize(0) 
  {
    if (aFileIndex >= 0)
    {
      ExtractStatuses.Reserve(1);
      ExtractStatuses.Add(true);
    }
  };
};

STDMETHODIMP CHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *_anExtractCallBack)
{
  COM_TRY_BEGIN
  bool aTestMode = (_aTestMode != 0);
  CComPtr<IExtractCallback200> anExtractCallBack = _anExtractCallBack;
  UINT64 anImportantTotalUnPacked = 0, anImportantTotalPacked = 0;
  UINT64 aCensoredTotalUnPacked = 0, aCensoredTotalPacked = 0;
  if(aNumItems == 0)
    return S_OK;

  CObjectVector<CExtractFolderInfo> anExtractFolderInfoVector;
  for(UINT64 anIndexIndex = 0; anIndexIndex < aNumItems; anIndexIndex++)
  {
    int aFileIndex = anIndexes[anIndexIndex];
    int aFolderIndex = m_Database.m_FileIndexToFolderIndexMap[aFileIndex];
    if (aFolderIndex < 0)
    {
      anExtractFolderInfoVector.Add(CExtractFolderInfo(aFileIndex, -1));
      continue;
    }
    if (anExtractFolderInfoVector.IsEmpty() || 
        aFolderIndex != anExtractFolderInfoVector.Back().FolderIndex)
      anExtractFolderInfoVector.Add(CExtractFolderInfo(-1, aFolderIndex));
    CExtractFolderInfo &anExtractFolderInfo = anExtractFolderInfoVector.Back();

    // const CFolderInfo &aFolderInfo = m_dam_Folders[aFolderIndex];
    UINT32 aStartIndex = m_Database.m_FolderStartFileIndex[aFolderIndex];
    for (UINT64 anIndex = anExtractFolderInfo.ExtractStatuses.Size();
        anIndex <= aFileIndex - aStartIndex; anIndex++)
    {
      UINT64 anUnPackSize = m_Database.m_Files[aStartIndex + anIndex].UnPackSize;
      anExtractFolderInfo.UnPackSize += anUnPackSize;
      anImportantTotalUnPacked += anUnPackSize;
      anExtractFolderInfo.ExtractStatuses.Add(anIndex == aFileIndex - aStartIndex);
    }
  }

  anExtractCallBack->SetTotal(anImportantTotalUnPacked);

  CDecoder aDecoder;

  UINT64 aCurrentImportantTotalUnPacked = 0;
  UINT64 aTotalFolderUnPacked;

  for(int i = 0; i < anExtractFolderInfoVector.Size(); i++, 
      aCurrentImportantTotalUnPacked += aTotalFolderUnPacked)
  {
    CExtractFolderInfo &anExtractFolderInfo = anExtractFolderInfoVector[i];
    aTotalFolderUnPacked = anExtractFolderInfo.UnPackSize;

    RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentImportantTotalUnPacked));

    CComObjectNoLock<CFolderOutStream> *aFolderOutStream = 
      new CComObjectNoLock<CFolderOutStream>;
    CComPtr<ISequentialOutStream> anOutStream(aFolderOutStream);

    UINT32 aStartIndex;
    if (anExtractFolderInfo.FileIndex >= 0)
      aStartIndex = anExtractFolderInfo.FileIndex;
    else
      aStartIndex = m_Database.m_FolderStartFileIndex[anExtractFolderInfo.FolderIndex];


    RETURN_IF_NOT_S_OK(aFolderOutStream->Init(&m_Database, aStartIndex, 
        &anExtractFolderInfo.ExtractStatuses, anExtractCallBack, aTestMode));

    if (anExtractFolderInfo.FileIndex >= 0)
      continue;

    UINT32 aFolderIndex = anExtractFolderInfo.FolderIndex;
    const CFolderItemInfo &aFolderInfo = m_Database.m_Folders[aFolderIndex];

    CObjectVector< CComPtr<ISequentialInStream> > anInStreams;

    CLockedInStream aLockedInStream;
    aLockedInStream.Init(m_InStream);


    UINT64 aFolderStartPackStreamIndex = m_Database.m_FolderStartPackStreamIndex[aFolderIndex];

    for (int j = 0; j < aFolderInfo.PackStreams.Size(); j++)
    {
      const CPackStreamInfo &aPackStreamInfo = aFolderInfo.PackStreams[j];
      CComObjectNoLock<CLockedSequentialInStreamImp> *aLockedStreamImpSpec = new 
        CComObjectNoLock<CLockedSequentialInStreamImp>;
      CComPtr<ISequentialInStream> aLockedStreamImp = aLockedStreamImpSpec;
      UINT64 aStreamStartPos = m_Database.GetFolderStreamPos(aFolderIndex, j);
      aLockedStreamImpSpec->Init(&aLockedInStream, aStreamStartPos);

      CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
        CComObjectNoLock<CLimitedSequentialInStream>;
      CComPtr<ISequentialInStream> anInStream = aStreamSpec;
      aStreamSpec->Init(aLockedStreamImp, 
          m_Database.m_PackSizes[aFolderStartPackStreamIndex + j]);
      anInStreams.Add(anInStream);
    }

    CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
    aLocalProgressSpec->Init(anExtractCallBack, false);

    CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
    aLocalCompressProgressSpec->Init(aProgress, NULL, &aCurrentImportantTotalUnPacked);

    UINT32 aPackStreamIndex = m_Database.m_FolderStartPackStreamIndex[aFolderIndex];
    UINT64 aFolderStartPackPos = m_Database.GetFolderStreamPos(aFolderIndex, 0);

    try
    {
      HRESULT aResult = aDecoder.Decode(m_InStream,
          aFolderStartPackPos, 
          &m_Database.m_PackSizes[aPackStreamIndex],
          aFolderInfo,
          anOutStream,
          aCompressProgress);

      if (aResult == S_FALSE)
        throw "data error";
      if (aResult != S_OK)
        return aResult;
      RETURN_IF_NOT_S_OK(aFolderOutStream->WasWritingFinished());
    }
    catch(...)
    {
      RETURN_IF_NOT_S_OK(aFolderOutStream->FlushCorrupted());
      continue;
    }
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::ExtractAllItems(INT32 aTestMode,
      IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> anIndexes;
  anIndexes.Reserve(m_Database.m_Files.Size());
  for(int i = 0; i < m_Database.m_Files.Size(); i++)
    anIndexes.Add(i);
  return Extract(&anIndexes.Front(), anIndexes.Size(), aTestMode,
      anExtractCallBack);
  COM_TRY_END
}
