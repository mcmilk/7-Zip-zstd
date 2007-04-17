// OutStreamWithCRC.cpp

#include "StdAfx.h"

#include "OutStreamWithCRC.h"

STDMETHODIMP COutStreamWithCRC::Write(const void *data,  UInt32 size, UInt32 *processedSize)
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
  if (_calculateCrc)
    _crc = CrcUpdate(_crc, data, realProcessedSize);
  _size += realProcessedSize;
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}
