// StreamObjects.cpp

#include "StdAfx.h"

#include "StreamObjects.h"

void COutStreamImp::Init()
{
  _size = 0;
}

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

void CInStreamImp::Init(BYTE *dataPointer, UINT32 size)
{
  _dataPointer = dataPointer;
  _size = size;
  _pos = 0;
}

STDMETHODIMP CInStreamImp::Read(void *data, ULONG size, ULONG *processedSize)
{
  UINT32 numBytesToRead = MyMin(_pos + size, _size) - _pos;
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



STDMETHODIMP CSequentialInStreamImp::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 numBytesToRead = MyMin(_pos + size, _size) - _pos;
  memmove(data, _dataPointer + _pos, numBytesToRead);
  _pos += numBytesToRead;
  if(processedSize != NULL)
    *processedSize = numBytesToRead;
  return S_OK;
}

STDMETHODIMP CSequentialInStreamImp::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  return Read(data, size, processedSize);
}

////////////////////


void CWriteBuffer::Write(const void *data, UINT32 size)
{
  size_t newCapacity = _size + size;
  _buffer.EnsureCapacity(newCapacity);
  memmove(_buffer + _size, data, size);
  _size += size;
}

STDMETHODIMP CSequentialOutStreamImp::Write(const void *data, UINT32 size, UINT32 *processedSize)
{
  _writeBuffer.Write(data, size);
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK; 
}

STDMETHODIMP CSequentialOutStreamImp::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
  return Write(data, size, processedSize);
}

STDMETHODIMP CSequentialOutStreamImp2::Write(const void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 newSize = size;
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

STDMETHODIMP CSequentialOutStreamImp2::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
  return Write(data, size, processedSize);
}



STDMETHODIMP CSequentialInStreamSizeCount::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = _stream->Read(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result; 
}

STDMETHODIMP CSequentialInStreamSizeCount::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = _stream->ReadPart(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result; 
}


STDMETHODIMP CSequentialOutStreamSizeCount::Write(const void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = _stream->Write(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result; 
}

STDMETHODIMP CSequentialOutStreamSizeCount::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = _stream->WritePart(data, size, &realProcessedSize);
  _size += realProcessedSize;
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return result; 
}
