// InStreamWithCRC.cpp

#include "StdAfx.h"

#include "InStreamWithCRC.h"

STDMETHODIMP CInStreamWithCRC::Read(void *data, 
    UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = _stream->Read(data, size, &realProcessedSize);
  _crc.Update(data, realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}

STDMETHODIMP CInStreamWithCRC::ReadPart(void *data, 
    UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = _stream->ReadPart(data, size, &realProcessedSize);
  _crc.Update(data, realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}

STDMETHODIMP CInStreamWithCRC::Seek(INT64 offset, 
    UINT32 seekOrigin, UINT64 *newPosition)
{
  if (seekOrigin != STREAM_SEEK_SET || offset != 0)
    return E_FAIL;
  _crc.Init();
  return _stream->Seek(offset, seekOrigin, newPosition);
}
