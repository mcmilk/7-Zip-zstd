// DummyOutStream.cpp

#include "StdAfx.h"

#include "DummyOutStream.h"

void CDummyOutStream::Init(ISequentialOutStream *outStream)
{
  m_Stream = outStream;
}

STDMETHODIMP CDummyOutStream::Write(const void *data, 
    UInt32 size, UInt32 *processedSize)
{
  if(m_Stream)
    return m_Stream->Write(data, size, processedSize);
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}
