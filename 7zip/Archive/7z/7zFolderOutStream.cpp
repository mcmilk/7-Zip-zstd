// 7zFolderOutStream.cpp

#include "StdAfx.h"

#include "7zFolderOutStream.h"

namespace NArchive {
namespace N7z {

CFolderOutStream::CFolderOutStream()
{
  _outStreamWithHashSpec = new COutStreamWithCRC;
  _outStreamWithHash = _outStreamWithHashSpec;
}

HRESULT CFolderOutStream::Init(
    CArchiveDatabaseEx *archiveDatabase,
    UINT32 startIndex,
    const CBoolVector *extractStatuses, 
    IArchiveExtractCallback *extractCallback,
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
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
  else
    askMode = NArchive::NExtract::NAskMode::kSkip;
  CMyComPtr<ISequentialOutStream> realOutStream;

  UINT32 index = _startIndex + _currentIndex;
  RINOK(_extractCallback->GetStream(index, &realOutStream, askMode));

  _outStreamWithHashSpec->Init(realOutStream);
  if (askMode == NArchive::NExtract::NAskMode::kExtract &&
      (!realOutStream)) 
  {
    UINT32 index = _startIndex + _currentIndex;
    const CFileItemInfo &fileInfo = _archiveDatabase->Files[index];
    if (!fileInfo.IsAnti && !fileInfo.IsDirectory)
      askMode = NArchive::NExtract::NAskMode::kSkip;
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
    RINOK(OpenFile());
    RINOK(_extractCallback->SetOperationResult(
        NArchive::NExtract::NOperationResult::kOK));
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
      RINOK(_outStreamWithHash->Write((const BYTE *)data + realProcessedSize, numBytesToWrite, &processedSizeLocal));

      _filePos += processedSizeLocal;
      realProcessedSize += processedSizeLocal;
      if (_filePos == fileSize)
      {
        bool digestsAreEqual;
        if (fileInfo.FileCRCIsDefined)
          digestsAreEqual = fileInfo.FileCRC == _outStreamWithHashSpec->GetCRC();
        else
          digestsAreEqual = true;

        RINOK(_extractCallback->SetOperationResult(
            digestsAreEqual ? 
            NArchive::NExtract::NOperationResult::kOK :
            NArchive::NExtract::NOperationResult::kCRCError));
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
      RINOK(OpenFile());
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

HRESULT CFolderOutStream::FlushCorrupted(INT32 resultEOperationResult)
{
  while(_currentIndex < _extractStatuses->Size())
  {
    if (_fileIsOpen)
    {
      RINOK(_extractCallback->SetOperationResult(resultEOperationResult));
      _outStreamWithHashSpec->ReleaseStream();
      _fileIsOpen = false;
      _currentIndex++;
    }
    else
    {
      RINOK(OpenFile());
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

}}
