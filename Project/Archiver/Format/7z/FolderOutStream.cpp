// FolderOutStream.cpp

#include "StdAfx.h"

#include "FolderOutStream.h"
#include "ItemInfoUtils.h"

#include "RegistryInfo.h"

#include "Common/Defs.h"

#include "Windows/Defs.h"

using namespace NArchive;
using namespace N7z;

CFolderOutStream::CFolderOutStream()
{
  m_OutStreamWithHashSpec = new CComObjectNoLock<COutStreamWithCRC>;
  m_OutStreamWithHash = m_OutStreamWithHashSpec;
}

HRESULT CFolderOutStream::Init(
    NArchive::N7z::CArchiveDatabaseEx *anArchiveDatabase,
    UINT32 aStartIndex,
    const CBoolVector *anExtractStatuses, 
    IExtractCallback200 *anExtractCallBack,
    bool aTestMode)
{
  m_ArchiveDatabase = anArchiveDatabase;
  m_StartIndex = aStartIndex;

  m_ExtractStatuses = anExtractStatuses;
  m_ExtractCallBack = anExtractCallBack;
  m_TestMode = aTestMode;

  m_CurrentIndex = 0;
  m_FileIsOpen = false;
  return WriteEmptyFiles();
}

HRESULT CFolderOutStream::OpenFile()
{
  INT32 anAskMode;
  if((*m_ExtractStatuses)[m_CurrentIndex])
    anAskMode = m_TestMode ? 
        NArchiveHandler::NExtract::NAskMode::kTest :
        NArchiveHandler::NExtract::NAskMode::kExtract;
  else
    anAskMode = NArchiveHandler::NExtract::NAskMode::kSkip;
  CComPtr<ISequentialOutStream> aRealOutStream;

  UINT32 anIndex = m_StartIndex + m_CurrentIndex;
  RETURN_IF_NOT_S_OK(m_ExtractCallBack->Extract(anIndex, &aRealOutStream, anAskMode));

  m_OutStreamWithHashSpec->Init(aRealOutStream);
  if (anAskMode == NArchiveHandler::NExtract::NAskMode::kExtract &&
      (!aRealOutStream)) 
    anAskMode = NArchiveHandler::NExtract::NAskMode::kSkip;
  return m_ExtractCallBack->PrepareOperation(anAskMode);
}

HRESULT CFolderOutStream::WriteEmptyFiles()
{
  for(;m_CurrentIndex < m_ExtractStatuses->Size(); m_CurrentIndex++)
  {
    UINT32 anIndex = m_StartIndex + m_CurrentIndex;
    const CFileItemInfo &aFileInfo = m_ArchiveDatabase->m_Files[anIndex];
    if (!aFileInfo.IsDirectory && aFileInfo.UnPackSize != 0)
      return S_OK;
    RETURN_IF_NOT_S_OK(OpenFile());
    RETURN_IF_NOT_S_OK(m_ExtractCallBack->OperationResult(
        NArchiveHandler::NExtract::NOperationResult::kOK));
    m_OutStreamWithHashSpec->ReleaseStream();
  }
  return S_OK;
}

STDMETHODIMP CFolderOutStream::Write(const void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal = 0;
  while(m_CurrentIndex < m_ExtractStatuses->Size())
  {
    if (m_FileIsOpen)
    {
      UINT32 anIndex = m_StartIndex + m_CurrentIndex;
      const CFileItemInfo &aFileInfo = m_ArchiveDatabase->m_Files[anIndex];
      UINT64 aFileSize = aFileInfo.UnPackSize;
      
      UINT32 aNumBytesToWrite = (UINT32)MyMin(aFileSize - m_FilePos, 
          UINT64(aSize - aProcessedSizeReal));
      
      UINT32 aProcessedSizeLocal;
      RETURN_IF_NOT_S_OK(m_OutStreamWithHash->Write((const BYTE *)aData + aProcessedSizeReal, aNumBytesToWrite, &aProcessedSizeLocal));

      m_FilePos += aProcessedSizeLocal;
      aProcessedSizeReal += aProcessedSizeLocal;
      if (m_FilePos == aFileSize)
      {
        bool aDigestsAreEqual;
        if (aFileInfo.FileCRCIsDefined)
          aDigestsAreEqual = aFileInfo.FileCRC == m_OutStreamWithHashSpec->GetCRC();
        else
          aDigestsAreEqual = true;

        RETURN_IF_NOT_S_OK(m_ExtractCallBack->OperationResult(
            aDigestsAreEqual ? 
            NArchiveHandler::NExtract::NOperationResult::kOK :
            NArchiveHandler::NExtract::NOperationResult::kCRCError));
        m_OutStreamWithHashSpec->ReleaseStream();
        m_FileIsOpen = false;
        m_CurrentIndex++;
      }
      if (aProcessedSizeReal == aSize)
      {
        if (aProcessedSize != NULL)
          *aProcessedSize = aProcessedSizeReal;
        return WriteEmptyFiles();
      }
    }
    else
    {
      RETURN_IF_NOT_S_OK(OpenFile());
      m_FileIsOpen = true;
      m_FilePos = 0;
    }
  }
  if (aProcessedSize != NULL)
    *aProcessedSize = aSize;
  return S_OK;
}

STDMETHODIMP CFolderOutStream::WritePart(const void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  return Write(aData, aSize, aProcessedSize);
}

HRESULT CFolderOutStream::FlushCorrupted()
{
  while(m_CurrentIndex < m_ExtractStatuses->Size())
  {
    if (m_FileIsOpen)
    {
      RETURN_IF_NOT_S_OK(m_ExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kDataError));
      m_OutStreamWithHashSpec->ReleaseStream();
      m_FileIsOpen = false;
      m_CurrentIndex++;
    }
    else
    {
      RETURN_IF_NOT_S_OK(OpenFile());
      m_FileIsOpen = true;
    }
  }
  return S_OK;
}

HRESULT CFolderOutStream::WasWritingFinished()
{
  if (m_CurrentIndex == m_ExtractStatuses->Size())
    return S_OK;
  return E_FAIL;
}
