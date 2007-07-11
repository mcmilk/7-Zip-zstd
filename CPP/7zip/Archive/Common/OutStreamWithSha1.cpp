// OutStreamWithSha1.cpp

#include "StdAfx.h"

#include "OutStreamWithSha1.h"

STDMETHODIMP COutStreamWithSha1::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result;
  if(!_stream)
  {
    realProcessedSize = size;
    result = S_OK;
  }
  else
    result = _stream->Write(data, size, &realProcessedSize);
  if (_calculate)
    _sha.Update((const Byte *)data, realProcessedSize);
  _size += realProcessedSize;
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}
