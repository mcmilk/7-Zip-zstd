// MultiStream.cpp

#include "StdAfx.h"

#include "MultiStream.h"
#include "MultiStream.h"

HRESULT CLockedInStream::Read(UINT64 aStartPos, void *aData, UINT32 aSize, 
    UINT32 *aProcessedSize)
{
  NWindows::NSynchronization::CSingleLock aLock(&m_CriticalSection, true);
  RETURN_IF_NOT_S_OK(m_Stream->Seek(aStartPos, STREAM_SEEK_SET, NULL));
  return m_Stream->Read(aData, aSize, aProcessedSize);
}

HRESULT CLockedInStream::ReadPart(UINT64 aStartPos, void *aData, UINT32 aSize, 
  UINT32 *aProcessedSize)
{
  NWindows::NSynchronization::CSingleLock aLock(&m_CriticalSection, true);
  RETURN_IF_NOT_S_OK(m_Stream->Seek(aStartPos, STREAM_SEEK_SET, NULL));
  return m_Stream->ReadPart(aData, aSize, aProcessedSize);
}


STDMETHODIMP CLockedSequentialInStreamImp::Read(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal = 0;
  HRESULT aResult = m_LockedInStream->Read(m_Pos, aData, aSize, &aProcessedSizeReal);
  m_Pos += aProcessedSizeReal;
  if (aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return aResult;
}

STDMETHODIMP CLockedSequentialInStreamImp::ReadPart(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal = 0;
  HRESULT aResult = m_LockedInStream->ReadPart(m_Pos, aData, aSize, &aProcessedSizeReal);
  m_Pos += aProcessedSizeReal;
  if (aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return aResult;
}
