// OffsetStream.cpp

#include "StdAfx.h"

#include "Interface/OffsetStream.h"
#include "Common/Defs.h"


HRESULT COffsetOutStream::Init(IOutStream *aStream, UINT64 anOffset)
{
  m_Offset = anOffset;
  m_Stream = aStream;
  return m_Stream->Seek(anOffset, STREAM_SEEK_SET, NULL);
}

STDMETHODIMP COffsetOutStream::Write(const void *aData, UINT32 aSize, 
    UINT32 *aProcessedSize)
{
  return m_Stream->Write(aData, aSize, aProcessedSize);
}

STDMETHODIMP COffsetOutStream::WritePart(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return m_Stream->WritePart(aData, aSize, aProcessedSize);
}

STDMETHODIMP COffsetOutStream::Seek(INT64 anOffset, UINT32 aSeekOrigin, 
    UINT64 *aNewPosition)
{
  UINT64 anAbsoluteNewPosition;
  if (aSeekOrigin == STREAM_SEEK_SET)
    anOffset += m_Offset;
  HRESULT aResult = m_Stream->Seek(anOffset, aSeekOrigin, &anAbsoluteNewPosition);
  if (aNewPosition != NULL)
    *aNewPosition = anAbsoluteNewPosition - m_Offset;
  return aResult;
}

STDMETHODIMP COffsetOutStream::SetSize(INT64 aNewSize)
{
  return m_Stream->SetSize(m_Offset + aNewSize);
}
