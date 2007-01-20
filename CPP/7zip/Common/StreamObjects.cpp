// StreamObjects.cpp

#include "StdAfx.h"

#include "StreamObjects.h"
#include "../../Common/Defs.h"


STDMETHODIMP CSequentialInStreamImp::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 numBytesToRead = (UInt32)(MyMin(_pos + size, _size) - _pos);
  memmove(data, _dataPointer + _pos, numBytesToRead);
  _pos += numBytesToRead;
  if(processedSize != NULL)
    *processedSize = numBytesToRead;
  return S_OK;
}


void CWriteBuffer::Write(const void *data, size_t size)
{
  size_t newCapacity = _size + size;
  _buffer.EnsureCapacity(newCapacity);
  memmove(_buffer + _size, data, size);
  _size += size;
}

STDMETHODIMP CSequentialOutStreamImp::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  _writeBuffer.Write(data, size);
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK; 
}

STDMETHODIMP CSequentialOutStreamImp2::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 newSize = size;
  if (_pos + size > _size)
    newSize = (UInt32)(_size - _pos);
  memmove(_buffer + _pos, data, newSize);
  if(processedSize != NULL)
    *processedSize = newSize;
  _pos += newSize;
  if (newSize != size)
    return E_FAIL;
  return S_OK; 
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
