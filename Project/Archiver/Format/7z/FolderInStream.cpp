// FolderInStream.cpp

#include "StdAfx.h"

#include "FolderInStream.h"

#include "Windows/Defs.h"
#include "Common/Defs.h"

using namespace NArchive;
using namespace N7z;

CFolderInStream::CFolderInStream()
{
  _inStreamWithHashSpec = new CComObjectNoLock<CInStreamWithCRC>;
  _inStreamWithHash = _inStreamWithHashSpec;
}

void CFolderInStream::Init(IUpdateCallBack *updateCallback, 
    const UINT32 *fileIndices, UINT32 numFiles)
{
  _updateCallback = updateCallback;
  _numFiles = numFiles;
  _fileIndex = 0;
  _fileIndices = fileIndices;
  CRCs.Clear();
  Sizes.Clear();
  _fileIsOpen = false;
  _currentSizeIsDefined = false;
}

HRESULT CFolderInStream::OpenStream()
{
  _filePos = 0;
  while (_fileIndex < _numFiles)
  {
    _currentSizeIsDefined = false;
    CComPtr<IInStream> stream;
    RETURN_IF_NOT_S_OK(_updateCallback->CompressOperation(
        _fileIndices[_fileIndex], &stream));
    _fileIndex++;
    _inStreamWithHashSpec->Init(stream);
    if (!stream)
    {
      RETURN_IF_NOT_S_OK(_updateCallback->OperationResult(NArchiveHandler::NUpdate::NOperationResult::kOK));
      Sizes.Add(0);
      AddDigest();
      continue;
    }
    CComPtr<IStreamGetSize> streamGetSize;
    if (stream.QueryInterface(&streamGetSize) == S_OK)
    {
      _currentSizeIsDefined = true;
      RETURN_IF_NOT_S_OK(streamGetSize->GetSize(&_currentSize));
    }

    _fileIsOpen = true;
    return S_OK;
  }
  return S_OK;
}

void CFolderInStream::AddDigest()
{
  CRCs.Add(_inStreamWithHashSpec->GetCRC());
}

HRESULT CFolderInStream::CloseStream()
{
  RETURN_IF_NOT_S_OK(_updateCallback->OperationResult(NArchiveHandler::NUpdate::NOperationResult::kOK));
  _inStreamWithHashSpec->ReleaseStream();
  _fileIsOpen = false;
  Sizes.Add(_filePos);
  AddDigest();
  return S_OK;
}

STDMETHODIMP CFolderInStream::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize = 0;
  while ((_fileIndex < _numFiles || _fileIsOpen) && size > 0)
  {
    if (_fileIsOpen)
    {
      UINT32 localProcessedSize;
      RETURN_IF_NOT_S_OK(_inStreamWithHash->Read(
          ((BYTE *)data) + realProcessedSize, size, &localProcessedSize));
      if (localProcessedSize == 0)
      {
        RETURN_IF_NOT_S_OK(CloseStream());
        continue;
      }
      realProcessedSize += localProcessedSize;
      _filePos += localProcessedSize;
      size -= localProcessedSize;
    }
    else
    {
      RETURN_IF_NOT_S_OK(OpenStream());
    }
  }
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return S_OK;
}

STDMETHODIMP CFolderInStream::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  return Read(data, size, processedSize);
}


STDMETHODIMP CFolderInStream::GetSubStreamSize(UINT64 subStream, UINT64 *value)
{
  *value = 0;
  if (subStream < Sizes.Size())
  {
    *value= Sizes[subStream];
    return S_OK;
  }
  if (subStream > Sizes.Size())
    return E_FAIL;
  if (!_currentSizeIsDefined)
    return S_FALSE;
  *value = _currentSize;
  return S_OK;
}
