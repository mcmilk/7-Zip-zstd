// FolderOutStream.cpp

#include "StdAfx.h"

#include "FolderOutStream.h"
#include "ItemInfoUtils.h"

#include "RegistryInfo.h"

#include "Common/Defs.h"

#include "Windows/Defs.h"

using namespace NArchive;
using namespace N7z;

CFolderOutStream::CFolderOutStream()
{
  _outStreamWithHashSpec = new CComObjectNoLock<COutStreamWithCRC>;
  _outStreamWithHash = _outStreamWithHashSpec;
}

HRESULT CFolderOutStream::Init(
    NArchive::N7z::CArchiveDatabaseEx *archiveDatabase,
    UINT32 startIndex,
    const CBoolVector *extractStatuses, 
    IExtractCallback200 *extractCallback,
    bool testMode)
{
  _archiveDatabase = archiveDatabase;
  _startIndex = startIndex;

  _extractStatuses = extractStatuses;
  _extractCallback = extractCallback;
  _testMode = testMode;

  _currentIndex = 0;
  _fileIsOpen = false;
  return WriteEmptyFiles();
}

HRESULT CFolderOutStream::OpenFile()
{
  INT32 askMode;
  if((*_extractStatuses)[_currentIndex])
    askMode = _testMode ? 
        NArchiveHandler::NExtract::NAskMode::kTest :
        NArchiveHandler::NExtract::NAskMode::kExtract;
  else
    askMode = NArchiveHandler::NExtract::NAskMode::kSkip;
  CComPtr<ISequentialOutStream> realOutStream;

  UINT32 index = _startIndex + _currentIndex;
  RETURN_IF_NOT_S_OK(_extractCallback->Extract(index, &realOutStream, askMode));

  _outStreamWithHashSpec->Init(realOutStream);
  if (askMode == NArchiveHandler::NExtract::NAskMode::kExtract &&
      (!realOutStream)) 
  {
    UINT32 index = _startIndex + _currentIndex;
    const CFileItemInfo &fileInfo = _archiveDatabase->Files[index];
    if (!fileInfo.IsAnti && !fileInfo.IsDirectory)
      askMode = NArchiveHandler::NExtract::NAskMode::kSkip;
  }
  return _extractCallback->PrepareOperation(askMode);
}

HRESULT CFolderOutStream::WriteEmptyFiles()
{
  for(;_currentIndex < _extractStatuses->Size(); _currentIndex++)
  {
    UINT32 index = _startIndex + _currentIndex;
    const CFileItemInfo &fileInfo = _archiveDatabase->Files[index];
    if (!fileInfo.IsAnti && !fileInfo.IsDirectory && fileInfo.UnPackSize != 0)
      return S_OK;
    RETURN_IF_NOT_S_OK(OpenFile());
    RETURN_IF_NOT_S_OK(_extractCallback->OperationResult(
        NArchiveHandler::NExtract::NOperationResult::kOK));
    _outStreamWithHashSpec->ReleaseStream();
  }
  return S_OK;
}

STDMETHODIMP CFolderOutStream::Write(const void *data, 
    UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize = 0;
  while(_currentIndex < _extractStatuses->Size())
  {
    if (_fileIsOpen)
    {
      UINT32 index = _startIndex + _currentIndex;
      const CFileItemInfo &fileInfo = _archiveDatabase->Files[index];
      UINT64 fileSize = fileInfo.UnPackSize;
      
      UINT32 numBytesToWrite = (UINT32)MyMin(fileSize - _filePos, 
          UINT64(size - realProcessedSize));
      
      UINT32 processedSizeLocal;
      RETURN_IF_NOT_S_OK(_outStreamWithHash->Write((const BYTE *)data + realProcessedSize, numBytesToWrite, &processedSizeLocal));

      _filePos += processedSizeLocal;
      realProcessedSize += processedSizeLocal;
      if (_filePos == fileSize)
      {
        bool digestsAreEqual;
        if (fileInfo.FileCRCIsDefined)
          digestsAreEqual = fileInfo.FileCRC == _outStreamWithHashSpec->GetCRC();
        else
          digestsAreEqual = true;

        RETURN_IF_NOT_S_OK(_extractCallback->OperationResult(
            digestsAreEqual ? 
            NArchiveHandler::NExtract::NOperationResult::kOK :
            NArchiveHandler::NExtract::NOperationResult::kCRCError));
        _outStreamWithHashSpec->ReleaseStream();
        _fileIsOpen = false;
        _currentIndex++;
      }
      if (realProcessedSize == size)
      {
        if (processedSize != NULL)
          *processedSize = realProcessedSize;
        return WriteEmptyFiles();
      }
    }
    else
    {
      RETURN_IF_NOT_S_OK(OpenFile());
      _fileIsOpen = true;
      _filePos = 0;
    }
  }
  if (processedSize != NULL)
    *processedSize = size;
  return S_OK;
}

STDMETHODIMP CFolderOutStream::WritePart(const void *data, 
    UINT32 size, UINT32 *processedSize)
{
  return Write(data, size, processedSize);
}

HRESULT CFolderOutStream::FlushCorrupted()
{
  while(_currentIndex < _extractStatuses->Size())
  {
    if (_fileIsOpen)
    {
      RETURN_IF_NOT_S_OK(_extractCallback->OperationResult(NArchiveHandler::NExtract::NOperationResult::kDataError));
      _outStreamWithHashSpec->ReleaseStream();
      _fileIsOpen = false;
      _currentIndex++;
    }
    else
    {
      RETURN_IF_NOT_S_OK(OpenFile());
      _fileIsOpen = true;
    }
  }
  return S_OK;
}

HRESULT CFolderOutStream::WasWritingFinished()
{
  if (_currentIndex == _extractStatuses->Size())
    return S_OK;
  return E_FAIL;
}
