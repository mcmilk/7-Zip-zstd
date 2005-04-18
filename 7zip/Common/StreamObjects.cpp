// StreamObjects.cpp

#include "StdAfx.h"

#include "StreamObjects.h"
#include "../../Common/Defs.h"

/*
STDMETHODIMP COutStreamImp::Read(void *data, ULONG size, ULONG *processedSize)
  {  return E_NOTIMPL; }

STDMETHODIMP COutStreamImp::Write(void const *data, ULONG size, ULONG *processedSize)
{
  size_t newCapacity = _size + size;
  _buffer.EnsureCapacity(newCapacity);
  memmove(_buffer + _size, data, size);
  if(processedSize != NULL)
    *processedSize = size;
  _size += size;
  return S_OK; 
}

void CInStreamImp::Init(Byte *dataPointer, size_t size)
{
  _dataPointer = dataPointer;
  _size = size;
  _pos = 0;
}

STDMETHODIMP CInStreamImp::Read(void *data, ULONG size, ULONG *processedSize)
{
  UInt32 numBytesToRead = MyMin(_pos + (UInt32)size, _size) - _pos;
  if(processedSize != NULL)
    *processedSize = numBytesToRead;
  memmove(data, _dataPointer + _pos, numBytesToRead);
  _pos += numBytesToRead;
  if(numBytesToRead == size)
    return S_OK;
  else
    return S_FALSE;
}

STDMETHODIMP CInStreamImp::Write(void const *data, ULONG size, ULONG *processedSize)
  {  return E_NOTIMPL; }
*/


STDMETHODIMP CSequentialInStreamImp::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 numBytesToRead = (UInt32)(MyMin(_pos + size, _size) - _pos);
  memmove(data, _dataPointer + _pos, numBytesToRead);
  _pos += numBytesToRead;
  if(processedSize != NULL)
    *processedSize = numBytesToRead;
  return S_OK;
}

STDMETHODIMP CSequentialInStreamImp::ReadPart(void *data, UInt32 size, UInt32 *processedSize)
{
  return Read(data, size, processedSize);
}

////////////////////


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

STDMETHODIMP CSequentialOutStreamImp::WritePart(const void *data, UInt32 size, UInt32 *processedSize)
{
  return Write(data, size, processedSize);
}

STDMETHODIMP CSequentialOutStreamImp2::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 newSize = size;
  if (_pos + size > _size)
    newSize = _size - _pos;
  memmove(_buffer + _pos, data, newSize);
  if(processedSize != NULL)
    *processedSize = newSize;
  _pos += newSize;
  if (newSize != size)
    return E_FAIL;
  return S_OK; 
}

STDMETHODIMP CSequentialOutStreamImp2::WritePart(const void *data, UInt32 size, UInt32 *processedSize)
{
  return Write(data, size, processedSize);
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

STDMETHODIMP CSequentialInStreamSizeCount::ReadPart(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = _stream->ReadPart(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result; 
}

STDMETHODIMP CSequentialInStreamRollback::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  HRESULT result = S_OK;
  UInt32 realProcessedSizeTotal = 0;
  while (size > 0)
  {
    UInt32 realProcessedSize = 0;
    result = ReadPart(data, size, &realProcessedSize);
    size -= realProcessedSize;
    data = ((Byte *)data + realProcessedSize);
    realProcessedSizeTotal += realProcessedSize;
    if (realProcessedSize == 0 || result != S_OK)
      break;
  }
  if (processedSize != 0)
    *processedSize = realProcessedSizeTotal;
  return result;
}

STDMETHODIMP CSequentialInStreamRollback::ReadPart(void *data, UInt32 size, UInt32 *processedSize)
{
  if (_currentPos != _currentSize)
  {
    UInt32 curSize = _currentSize - _currentPos;
    if (size > curSize)
      size = curSize;
    memmove(data, _buffer + _currentPos, size);
    _currentPos += size;
    if (processedSize != 0)
      *processedSize = size;
    return S_OK;
  }
  UInt32 realProcessedSize;
  if (size > _bufferSize)
    size = _bufferSize;
  HRESULT result = _stream->ReadPart(_buffer, size, &realProcessedSize);
  memmove(data, _buffer, realProcessedSize);
  _size += realProcessedSize;
  _currentSize = realProcessedSize;
  _currentPos = realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result; 
}

HRESULT CSequentialInStreamRollback::Rollback(size_t rollbackSize)
{
  if (rollbackSize > _currentPos)
    return E_INVALIDARG;
  _currentPos -= rollbackSize;
  return S_OK;
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

STDMETHODIMP CSequentialOutStreamSizeCount::WritePart(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = _stream->WritePart(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result; 
}
