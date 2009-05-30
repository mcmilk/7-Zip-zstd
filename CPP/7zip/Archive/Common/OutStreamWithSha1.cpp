// OutStreamWithSha1.cpp

#include "StdAfx.h"

#include "OutStreamWithSha1.h"

STDMETHODIMP COutStreamWithSha1::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  HRESULT result = S_OK;
  if (_stream)
    result = _stream->Write(data, size, &size);
  if (_calculate)
    _sha.Update((const Byte *)data, size);
  _size += size;
  if (processedSize != NULL)
    *processedSize = size;
  return result;
}
