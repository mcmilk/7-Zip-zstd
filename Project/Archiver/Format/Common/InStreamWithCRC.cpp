// InStreamWithCRC.cpp

#include "StdAfx.h"

#include "InStreamWithCRC.h"

STDMETHODIMP CInStreamWithCRC::Read(void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->Read(aData, aSize, &aProcessedSizeReal);
  m_Crc.Update(aData, aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return aResult;
}

STDMETHODIMP CInStreamWithCRC::ReadPart(void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->ReadPart(aData, aSize, &aProcessedSizeReal);
  m_Crc.Update(aData, aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return aResult;
}

STDMETHODIMP CInStreamWithCRC::Seek(INT64 anOffset, 
    UINT32 aSeekOrigin, UINT64 *aNewPosition)
{
  if (aSeekOrigin != STREAM_SEEK_SET || anOffset != 0)
    return E_FAIL;
  m_Crc.Init();
  return m_Stream->Seek(anOffset, aSeekOrigin, aNewPosition);
}
