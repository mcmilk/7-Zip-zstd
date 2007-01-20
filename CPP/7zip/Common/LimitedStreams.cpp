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

