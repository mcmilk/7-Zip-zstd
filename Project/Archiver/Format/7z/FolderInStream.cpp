// FolderInStream.cpp

#include "StdAfx.h"

#include "FolderInStream.h"

#include "Windows/Defs.h"
#include "Common/Defs.h"

using namespace NArchive;
using namespace N7z;

CFolderInStream::CFolderInStream()
{
  m_InStreamWithHashSpec = new CComObjectNoLock<CInStreamWithCRC>;
  m_InStreamWithHash = m_InStreamWithHashSpec;
}

void CFolderInStream::Init(IUpdateCallBack *anUpdateCallBack, 
    const UINT32 *aFileIndexes, UINT32 aNumFiles)
{
  m_UpdateCallBack = anUpdateCallBack;
  m_NumFiles = aNumFiles;
  m_FileIndex = 0;
  m_FileIndexes = aFileIndexes;
  m_CRCs.Clear();
  m_Sizes.Clear();
  m_FileIsOpen = false;
  m_CurrentSizeIsDefined = false;
}

HRESULT CFolderInStream::OpenStream()
{
  m_FilePos = 0;
  while (m_FileIndex < m_NumFiles)
  {
    m_CurrentSizeIsDefined = false;
    CComPtr<IInStream> aStream;
    RETURN_IF_NOT_S_OK(m_UpdateCallBack->CompressOperation(
        m_FileIndexes[m_FileIndex], &aStream));
    m_FileIndex++;
    m_InStreamWithHashSpec->Init(aStream);
    if (!aStream)
    {
      RETURN_IF_NOT_S_OK(m_UpdateCallBack->OperationResult(NArchiveHandler::NUpdate::NOperationResult::kOK));
      m_Sizes.Add(0);
      AddDigest();
      continue;
    }
    CComPtr<IStreamGetSize> aStreamGetSize;
    if (aStream.QueryInterface(&aStreamGetSize) == S_OK)
    {
      m_CurrentSizeIsDefined = true;
      RETURN_IF_NOT_S_OK(aStreamGetSize->GetSize(&m_CurrentSize));
    }

    m_FileIsOpen = true;
    return S_OK;
  }
  return S_OK;
}

void CFolderInStream::AddDigest()
{
  m_CRCs.Add(m_InStreamWithHashSpec->GetCRC());
}

HRESULT CFolderInStream::CloseStream()
{
  RETURN_IF_NOT_S_OK(m_UpdateCallBack->OperationResult(NArchiveHandler::NUpdate::NOperationResult::kOK));
  m_InStreamWithHashSpec->ReleaseStream();
  m_FileIsOpen = false;
  m_Sizes.Add(m_FilePos);
  AddDigest();
  return S_OK;
}

STDMETHODIMP CFolderInStream::Read(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aRealProcessedSize = 0;
  while ((m_FileIndex < m_NumFiles || m_FileIsOpen) && aSize > 0)
  {
    if (m_FileIsOpen)
    {
      UINT32 aProcessedSizeLoc;
      RETURN_IF_NOT_S_OK(m_InStreamWithHash->Read(
          ((BYTE *)aData) + aRealProcessedSize, aSize, &aProcessedSizeLoc));
      if (aProcessedSizeLoc == 0)
      {
        RETURN_IF_NOT_S_OK(CloseStream());
        continue;
      }
      aRealProcessedSize += aProcessedSizeLoc;
      m_FilePos += aProcessedSizeLoc;
      aSize -= aProcessedSizeLoc;
    }
    else
    {
      RETURN_IF_NOT_S_OK(OpenStream());
    }
  }
  if (aProcessedSize != 0)
    *aProcessedSize = aRealProcessedSize;
  return S_OK;
}

STDMETHODIMP CFolderInStream::ReadPart(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return Read(aData, aSize, aProcessedSize);
}


STDMETHODIMP CFolderInStream::GetSubStreamSize(UINT64 aSubStream, UINT64 *aValue)
{
  *aValue = 0;
  if (aSubStream < m_Sizes.Size())
  {
    *aValue= m_Sizes[aSubStream];
    return S_OK;
  }
  if (aSubStream > m_Sizes.Size())
    return E_FAIL;
  if (!m_CurrentSizeIsDefined)
    return S_FALSE;
  *aValue = m_CurrentSize;
  return S_OK;
}
