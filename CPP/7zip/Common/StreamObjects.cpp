// StreamObjects.cpp

#include "StdAfx.h"

#include "StreamObjects.h"
#include "../../Common/Defs.h"


STDMETHODIMP CSequentialInStreamImp::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  size_t rem = _size - _pos;
  if (size < rem)
    rem = (size_t)size;
  memcpy(data, _dataPointer + _pos, rem);
  _pos += rem;
  if (processedSize != NULL)
    *processedSize = (UInt32)rem;
  return S_OK;
}


void CWriteBuffer::Write(const void *data, size_t size)
{
  size_t newCapacity = _size + size;
  _buffer.EnsureCapacity(newCapacity);
  memcpy(_buffer + _size, data, size);
  _size += size;
}

STDMETHODIMP CSequentialOutStreamImp::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  _writeBuffer.Write(data, (size_t)size);
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}

STDMETHODIMP CSequentialOutStreamImp2::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  size_t rem = _size - _pos;
  if (size < rem)
    rem = (size_t)size;
  memcpy(_buffer + _pos, data, rem);
  _pos += rem;
  if (processedSize != NULL)
    *processedSize = (UInt32)rem;
  return (rem == size ? S_OK : E_FAIL);
}

STDMETHODIMP CSequentialInStreamSizeCount::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = _stream->Read(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result;
}

STDMETHODIMP CSequentialOutStreamSizeCount::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = _stream->Write(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result;
}
