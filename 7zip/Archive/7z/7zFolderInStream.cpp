// 7zFolderInStream.cpp

#include "StdAfx.h"

#include "7zFolderInStream.h"

namespace NArchive {
namespace N7z {

CFolderInStream::CFolderInStream()
{
  _inStreamWithHashSpec = new CInStreamWithCRC;
  _inStreamWithHash = _inStreamWithHashSpec;
}

void CFolderInStream::Init(IArchiveUpdateCallback *updateCallback, 
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
    CMyComPtr<IInStream> stream;
    RINOK(_updateCallback->GetStream(_fileIndices[_fileIndex], &stream));
    _fileIndex++;
    _inStreamWithHashSpec->Init(stream);
    if (!stream)
    {
      RINOK(_updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
      Sizes.Add(0);
      AddDigest();
      continue;
    }
    CMyComPtr<IStreamGetSize> streamGetSize;
    if (stream.QueryInterface(IID_IStreamGetSize, &streamGetSize) == S_OK)
    {
      if(streamGetSize)
      {
        _currentSizeIsDefined = true;
        RINOK(streamGetSize->GetSize(&_currentSize));
      }
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
  RINOK(_updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK));
  _inStreamWithHashSpec->ReleaseStream();
  _fileIsOpen = false;
  Sizes.Add(_filePos);
  AddDigest();
  return S_OK;
}

STDMETHODIMP CFolderInStream::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize = 0;
  while ((_fileIndex < _numFiles || _fileIsOpen) && size > 0)
  {
    if (_fileIsOpen)
    {
      UINT32 localProcessedSize;
      RINOK(_inStreamWithHash->Read(
          ((BYTE *)data) + realProcessedSize, size, &localProcessedSize));
      if (localProcessedSize == 0)
      {
        RINOK(CloseStream());
        continue;
      }
      realProcessedSize += localProcessedSize;
      _filePos += localProcessedSize;
      size -= localProcessedSize;
      break;
    }
    else
    {
      RINOK(OpenStream());
    }
  }
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return S_OK;
}

STDMETHODIMP CFolderInStream::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize = 0;
  while (size > 0)
  {
    UINT32 localProcessedSize;
    RINOK(ReadPart(((BYTE *)data) + realProcessedSize, size, &localProcessedSize));
    if (localProcessedSize == 0)
      break;
    size -= localProcessedSize;
    realProcessedSize += localProcessedSize;
  }
  if (processedSize != 0)
    *processedSize = realProcessedSize;
  return S_OK;
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

}}