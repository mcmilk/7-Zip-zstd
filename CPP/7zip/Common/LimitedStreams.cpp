// LimitedStreams.cpp

#include "StdAfx.h"

#include "LimitedStreams.h"
#include "../../Common/Defs.h"

STDMETHODIMP CLimitedSequentialInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize = 0;
  UInt32 sizeToRead = (UInt32)MyMin((_size - _pos), (UInt64)size);
  HRESULT result = S_OK;
  if (sizeToRead > 0)
  {
    result = _stream->Read(data, sizeToRead, &realProcessedSize);
    _pos += realProcessedSize;
    if (realProcessedSize == 0)
      _wasFinished = true;
  }
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}

STDMETHODIMP CLimitedSequentialOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  HRESULT result = S_OK;
  if (processedSize != NULL)
    *processedSize = 0;
  if (size > _size)
  {
    size = (UInt32)_size;
    if (size == 0)
    {
      _overflow = true;
      return E_FAIL;
    }
  }
  if (_stream)
    result = _stream->Write(data, size, &size);
  _size -= size;
  if (processedSize != NULL)
    *processedSize = size;
  return result;
}
