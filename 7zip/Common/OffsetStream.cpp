// OffsetStream.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "OffsetStream.h"

HRESULT COffsetOutStream::Init(IOutStream *stream, UINT64 offset)
{
  _offset = offset;
  _stream = stream;
  return _stream->Seek(offset, STREAM_SEEK_SET, NULL);
}

STDMETHODIMP COffsetOutStream::Write(const void *data, UINT32 size, 
    UINT32 *processedSize)
{
  return _stream->Write(data, size, processedSize);
}

STDMETHODIMP COffsetOutStream::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
  return _stream->WritePart(data, size, processedSize);
}

STDMETHODIMP COffsetOutStream::Seek(INT64 offset, UINT32 seekOrigin, 
    UINT64 *newPosition)
{
  UINT64 absoluteNewPosition;
  if (seekOrigin == STREAM_SEEK_SET)
    offset += _offset;
  HRESULT result = _stream->Seek(offset, seekOrigin, &absoluteNewPosition);
  if (newPosition != NULL)
    *newPosition = absoluteNewPosition - _offset;
  return result;
}

STDMETHODIMP COffsetOutStream::SetSize(INT64 newSize)
{
  return _stream->SetSize(_offset + newSize);
}
