// StreamObjects2.cpp

#include "StdAfx.h"

#include "StreamObjects2.h"


STDMETHODIMP CSequentialInStreamSizeCount2::Read(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->Read(aData, aSize, &aProcessedSizeReal);
  m_Size += aProcessedSizeReal;
  if (aProcessedSize != 0)
    *aProcessedSize = aProcessedSizeReal;
  return aResult; 
}

STDMETHODIMP CSequentialInStreamSizeCount2::ReadPart(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->ReadPart(aData, aSize, &aProcessedSizeReal);
  m_Size += aProcessedSizeReal;
  if (aProcessedSize != 0)
    *aProcessedSize = aProcessedSizeReal;
  return aResult; 
}

STDMETHODIMP CSequentialInStreamSizeCount2::GetSubStreamSize(UINT64 aSubStream, UINT64 *aValue)
{
  if (m_GetSubStreamSize == NULL)
    return E_NOTIMPL;
  return  m_GetSubStreamSize->GetSubStreamSize(aSubStream, aValue);
}

