// MultiStream.cpp

#include "StdAfx.h"

#include "MultiStream.h"

HRESULT CLockedInStream::Read(UINT64 startPos, void *data, UINT32 size, 
    UINT32 *processedSize)
{
  NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
  RINOK(_stream->Seek(startPos, STREAM_SEEK_SET, NULL));
  return _stream->Read(data, size, processedSize);
}

HRESULT CLockedInStream::ReadPart(UINT64 startPos, void *data, UINT32 size, 
  UINT32 *processedSize)
{
  NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
  RINOK(_stream->Seek(startPos, STREAM_SEEK_SET, NULL));
  return _stream->ReadPart(data, size, processedSize);
}


STDMETHODIMP CLockedSequentialInStreamImp::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize = 0;
  HRESULT result = _lockedInStream->Read(_pos, data, size, &realProcessedSize);
  _pos += realProcessedSize;
  if (processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}

STDMETHODIMP CLockedSequentialInStreamImp::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize = 0;
  HRESULT result = _lockedInStream->ReadPart(_pos, data, size, &realProcessedSize);
  _pos += realProcessedSize;
  if (processedSize != NULL)
    *processedSize = realProcessedSize;
  return result;
}
