// LimitedStreams.cpp

#include "StdAfx.h"

#include "LimitedStreams.h"
#include "../../Common/Defs.h"

void CLimitedSequentialInStream::Init(ISequentialInStream *stream, UINT64 streamSize)
{
  _stream = stream;
  _size = streamSize;
}

STDMETHODIMP CLimitedSequentialInStream::Read(void *data, 
    UINT32 size, UINT32 *processedSize)
{
  UINT32 processedSizeReal;
  UINT32 sizeToRead = UINT32(MyMin(_size, UINT64(size)));
  HRESULT result = _stream->Read(data, sizeToRead, &processedSizeReal);
  _size -= processedSizeReal;
  if(processedSize != NULL)
    *processedSize = processedSizeReal;
  return result;
}
  
STDMETHODIMP CLimitedSequentialInStream::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 processedSizeReal;
  UINT32 sizeToRead = UINT32(MyMin(_size, UINT64(size)));
  HRESULT result = _stream->ReadPart(data, sizeToRead, &processedSizeReal);
  _size -= processedSizeReal;
  if(processedSize != NULL)
    *processedSize = processedSizeReal;
  return result;
}

