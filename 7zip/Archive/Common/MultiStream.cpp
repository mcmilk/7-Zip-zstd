// MultiStream.cpp

#include "StdAfx.h"

#include "MultiStream.h"

STDMETHODIMP CMultiStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if(processedSize != NULL)
    *processedSize = 0;
  while(_streamIndex < Streams.Size() && size > 0)
  {
    CSubStreamInfo &s = Streams[_streamIndex];
    if (_pos == s.Size)
    {
      _streamIndex++;
      _pos = 0;
      continue;
    }
    RINOK(s.Stream->Seek(s.Pos + _pos, STREAM_SEEK_SET, 0));
    UInt32 sizeToRead = UInt32(MyMin((UInt64)size, s.Size - _pos));
    UInt32 realProcessed;
    HRESULT result = s.Stream->Read(data, sizeToRead, &realProcessed);
    data = (void *)((Byte *)data + realProcessed);
    size -= realProcessed;
    if(processedSize != NULL)
      *processedSize += realProcessed;
    _pos += realProcessed;
    _seekPos += realProcessed;
    RINOK(result);
    if (realProcessed == 0)
      break;
  }
  return S_OK;
}
  
STDMETHODIMP CMultiStream::ReadPart(void *data, UInt32 size, UInt32 *processedSize)
{
  return Read(data, size, processedSize);
}

STDMETHODIMP CMultiStream::Seek(Int64 offset, UInt32 seekOrigin, 
    UInt64 *newPosition)
{
  UInt64 newPos;
  if(seekOrigin >= 3)
    return STG_E_INVALIDFUNCTION;
  switch(seekOrigin)
  {
    case STREAM_SEEK_SET:
      newPos = offset;
      break;
    case STREAM_SEEK_CUR:
      newPos = _seekPos + offset;
      break;
    case STREAM_SEEK_END:
      newPos = _totalLength + offset;
      break;
  }
  _seekPos = 0;
  for (_streamIndex = 0; _streamIndex < Streams.Size(); _streamIndex++)
  {
    UInt64 size = Streams[_streamIndex].Size;
    if (newPos < _seekPos + size)
    {
      _pos = newPos - _seekPos;
      _seekPos += _pos;
      if (newPosition != 0)
        *newPosition = newPos;
      return S_OK;
    }
    _seekPos += size;
  }
  if (newPos == _seekPos)
  {
    if (newPosition != 0)
      *newPosition = newPos;
    return S_OK;
  }
  return E_FAIL;
}


