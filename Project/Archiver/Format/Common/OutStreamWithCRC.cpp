// OutStreamWithCRC.cpp

#include "StdAfx.h"

#include "OutStreamWithCRC.h"

void COutStreamWithCRC::Init(ISequentialOutStream *aStream)
{
  m_Stream = aStream;
  InitCRC();
}

void COutStreamWithCRC::InitCRC()
{
  m_Crc.Init();
}

STDMETHODIMP COutStreamWithCRC::Write(const void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  HRESULT aResult;
  UINT32 aProcessedSizeReal;
  if(!m_Stream)
  {
    aProcessedSizeReal = aSize;
    aResult = S_OK;
  }
  else
    aResult = m_Stream->Write(aData, aSize, &aProcessedSizeReal);
  m_Crc.Update(aData, aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return aResult;
}

STDMETHODIMP COutStreamWithCRC::WritePart(const void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult;
  if(!m_Stream)
  {
    aProcessedSizeReal = aSize;
    aResult = S_OK;
  }
  else
    aResult = m_Stream->WritePart(aData, aSize, &aProcessedSizeReal);
  m_Crc.Update(aData, aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return aResult;
}
