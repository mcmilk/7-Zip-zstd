// 7zSpecStream.cpp

#include "StdAfx.h"

#include "7zSpecStream.h"

STDMETHODIMP CSequentialInStreamSizeCount2::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = _stream->Read(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result; 
}

STDMETHODIMP CSequentialInStreamSizeCount2::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = _stream->ReadPart(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result; 
}

STDMETHODIMP CSequentialInStreamSizeCount2::GetSubStreamSize(
    UINT64 subStream, UINT64 *value)
{
  if (_getSubStreamSize == NULL)
    return E_NOTIMPL;
  return  _getSubStreamSize->GetSubStreamSize(subStream, value);
}

