// DummyOutStream.cpp

#include "StdAfx.h"

#include "DummyOutStream.h"

void CDummyOutStream::Init(ISequentialOutStream *outStream)
{
  m_Stream = outStream;
}

STDMETHODIMP CDummyOutStream::Write(const void *data, 
    UINT32 size, UINT32 *processedSize)
{
  if(m_Stream)
    return m_Stream->Write(data, size, processedSize);
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}

STDMETHODIMP CDummyOutStream::WritePart(const void *data, 
    UINT32 size, UINT32 *processedSize)
{
  if(m_Stream)
    return m_Stream->WritePart(data, size, processedSize);
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}
