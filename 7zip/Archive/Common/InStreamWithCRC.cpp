// InStreamWithCRC.cpp

#include "StdAfx.h"

#include "InStreamWithCRC.h"

STDMETHODIMP CSequentialInStreamWithCRC::Read(void *data, 
    UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = _stream->Read(data, size, &realProcessedSize);
  _size += realProcessedSize;
  _crc.Update(data, realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}

STDMETHODIMP CInStreamWithCRC::Read(void *data, 
    UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = _stream->Read(data, size, &realProcessedSize);
  _size += realProcessedSize;
  _crc.Update(data, realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}

STDMETHODIMP CInStreamWithCRC::Seek(Int64 offset, 
    UInt32 seekOrigin, UInt64 *newPosition)
{
  if (seekOrigin != STREAM_SEEK_SET || offset != 0)
    return E_FAIL;
  _size = 0;
  _crc.Init();
  return _stream->Seek(offset, seekOrigin, newPosition);
}
