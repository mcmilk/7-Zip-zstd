// DummyOutStream.cpp

#include "StdAfx.h"

#include "DummyOutStream.h"

void CDummyOutStream::Init(ISequentialOutStream *aStream)
{
  m_Stream = aStream;
}

STDMETHODIMP CDummyOutStream::Write(const void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  if(m_Stream)
    return m_Stream->Write(aData, aSize, aProcessedSize);
  if(aProcessedSize != NULL)
    *aProcessedSize = aSize;
  return S_OK;
}

STDMETHODIMP CDummyOutStream::WritePart(const void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  if(m_Stream)
    return m_Stream->WritePart(aData, aSize, aProcessedSize);
  if(aProcessedSize != NULL)
    *aProcessedSize = aSize;
  return S_OK;
}
