// LimitedStreams.cpp

#include "StdAfx.h"

#include "Interface/LimitedStreams.h"
#include "Common/Defs.h"

void CLimitedSequentialInStream::Init(ISequentialInStream *aStream, UINT64 aStreamSize)
{
  m_Stream = aStream;
  m_Size = aStreamSize;
}

STDMETHODIMP CLimitedSequentialInStream::Read(void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  UINT32 aSizeToRead = UINT32(MyMin(m_Size, UINT64(aSize)));
  HRESULT aResult = m_Stream->Read(aData, aSizeToRead, &aProcessedSizeReal);
  m_Size -= aProcessedSizeReal;
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return aResult;
}
  
STDMETHODIMP CLimitedSequentialInStream::ReadPart(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  UINT32 aSizeToRead = UINT32(MyMin(m_Size, UINT64(aSize)));
  HRESULT aResult = m_Stream->ReadPart(aData, aSizeToRead, &aProcessedSizeReal);
  m_Size -= aProcessedSizeReal;
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  return aResult;
}

