// ArchiveExtractCallback.cpp

#include "StdAfx.h"

#include "ArchiveExtractCallback.h"

#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Time.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "Windows/PropVariantConversions.h"

#include "../../Common/FilePathAutoRename.h"

#include "../Common/ExtractingFilePath.h"

using namespace NWindows;

static const wchar_t *kCantAutoRename = L"ERROR: Can not create file with auto name";
static const wchar_t *kCantRenameFile = L"ERROR: Can not rename existing file ";
static const wchar_t *kCantDeleteOutputFile = L"ERROR: Can not delete output file ";


void CArchiveExtractCallback::Init(
    IInArchive *archiveHandler,
    IFolderArchiveExtractCallback *extractCallback2,
    bool stdOutMode,
    const UString &directoryPath, 
    NExtract::NPathMode::EEnum pathMode,
    NExtract::NOverwriteMode::EEnum overwriteMode,
    const UStringVector &removePathParts,
    const UString &itemDefaultName,
    const FILETIME &utcLastWriteTimeDefault,
    UInt32 attributesDefault)
{
  _stdOutMode = stdOutMode;
  _numErrors = 0;
  _extractCallback2 = extractCallback2;
  _itemDefaultName = itemDefaultName;
  _utcLastWriteTimeDefault = utcLastWriteTimeDefault;
  _attributesDefault = attributesDefault;
  _removePathParts = removePathParts;
  _pathMode = pathMode;
  _overwriteMode = overwriteMode;
  _archiveHandler = archiveHandler;
  _directoryPath = directoryPath;
  NFile::NName::NormalizeDirPathPrefix(_directoryPath);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 size)
{
  return _extractCallback2->SetTotal(size);
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 *completeValue)
{
  return _extractCallback2->SetCompleted(completeValue);
}

void CArchiveExtractCallback::CreateComplexDirectory(const UStringVector &dirPathParts)
{
  UString fullPath = _directoryPath;
  for(int i = 0; i < dirPathParts.Size(); i++)
  {
    fullPath += dirPathParts[i];
    NFile::NDirectory::MyCreateDirectory(fullPath);
    fullPath += wchar_t(NFile::NName::kDirDelimiter);
  }
}

static UString MakePathNameFromParts(const UStringVector &parts)
{
  UString result;
  for(int i = 0; i < parts.Size(); i++)
  {
    if(i != 0)
      result += wchar_t(NFile::NName::kDirDelimiter);
    result += parts[i];
  }
  return result;
}


STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index, 
    ISequentialOutStream **outStream, Int32 askExtractMode)
{
  *outStream = 0;
  _outFileStream.Release();
  NCOM::CPropVariant propVariant;
  RINOK(_archiveHandler->GetProperty(index, kpidPath, &propVariant));
  
  UString fullPath;
  if(propVariant.vt == VT_EMPTY)
    fullPath = _itemDefaultName;
  else 
  {
    if(propVariant.vt != VT_BSTR)
      return E_FAIL;
    fullPath = propVariant.bstrVal;
  }

  UString fullPathCorrect = GetCorrectPath(fullPath);
  _filePath = fullPath;
  _isSplit = false;

  RINOK(_archiveHandler->GetProperty(index, kpidPosition, &propVariant));
  if (propVariant.vt != VT_EMPTY)
  {
    if (propVariant.vt != VT_UI8)
      return E_FAIL;
    _position = propVariant.uhVal.QuadPart;
    _isSplit = true;
  }

  if(askExtractMode == NArchive::NExtract::NAskMode::kExtract)
  {
    if (_stdOutMode)
    {
      CMyComPtr<ISequentialOutStream> outStreamLoc = new CStdOutFileStream;
      *outStream = outStreamLoc.Detach();
      return S_OK;
    }

    RINOK(_archiveHandler->GetProperty(index, kpidAttributes, &propVariant));
    if (propVariant.vt == VT_EMPTY)
    {
      _processedFileInfo.Attributes = _attributesDefault;
      _processedFileInfo.AttributesAreDefined = false;
    }
    else
    {
      if (propVariant.vt != VT_UI4)
        throw "incorrect item";
      _processedFileInfo.Attributes = propVariant.ulVal;
      _processedFileInfo.AttributesAreDefined = true;
    }

    RINOK(_archiveHandler->GetProperty(index, kpidIsFolder, &propVariant));
    _processedFileInfo.IsDirectory = VARIANT_BOOLToBool(propVariant.boolVal);

    RINOK(_archiveHandler->GetProperty(index, kpidLastWriteTime, &propVariant));
    switch(propVariant.vt)
    {
      case VT_EMPTY:
        _processedFileInfo.UTCLastWriteTime = _utcLastWriteTimeDefault;
        break;
      case VT_FILETIME:
        _processedFileInfo.UTCLastWriteTime = propVariant.filetime;
        break;
      default:
        return E_FAIL;
    }

    RINOK(_archiveHandler->GetProperty(index, kpidSize, &propVariant));
    bool newFileSizeDefined = (propVariant.vt != VT_EMPTY);
    UInt64 newFileSize;
    if (newFileSizeDefined)
      newFileSize = ConvertPropVariantToUINT64(propVariant);

    bool isAnti = false;
    {
      NCOM::CPropVariant propVariantTemp;
      RINOK(_archiveHandler->GetProperty(index, kpidIsAnti, 
          &propVariantTemp));
      if (propVariantTemp.vt == VT_BOOL)
        isAnti = VARIANT_BOOLToBool(propVariantTemp.boolVal);
    }

    UStringVector pathParts; 
    SplitPathToParts(fullPathCorrect, pathParts);
    // SplitPathToParts(fullPath, pathParts);
    
    if(pathParts.IsEmpty())
      return E_FAIL;
    UString processedPath;
    switch(_pathMode)
    {
      case NExtract::NPathMode::kFullPathnames:
      {
        processedPath = fullPathCorrect;
        // processedPath = GetCorrectPath(fullPath);
        break;
      }
      case NExtract::NPathMode::kCurrentPathnames:
      {
        int numRemovePathParts = _removePathParts.Size();
        if(pathParts.Size() <= numRemovePathParts)
          return E_FAIL;
        for(int i = 0; i < numRemovePathParts; i++)
          if(_removePathParts[i].CollateNoCase(pathParts[i]) != 0)
            return E_FAIL;
        pathParts.Delete(0, numRemovePathParts);
        processedPath = MakePathNameFromParts(pathParts);
        processedPath = GetCorrectPath(processedPath);
        break;
      }
      case NExtract::NPathMode::kNoPathnames:
      {
        processedPath = pathParts.Back(); 
        pathParts.Delete(0, pathParts.Size() - 1); // Test it!!
        break;
      }
    }
    if(!_processedFileInfo.IsDirectory)
      pathParts.DeleteBack();

    for(int i = 0; i < pathParts.Size(); i++)
      pathParts[i] = GetCorrectFileName(pathParts[i]);
    
    if (!isAnti)
      if (!pathParts.IsEmpty())
        CreateComplexDirectory(pathParts);

    UString fullProcessedPath = _directoryPath + processedPath;

    if(_processedFileInfo.IsDirectory)
    {
      _diskFilePath = fullProcessedPath;
      if (isAnti)
        NFile::NDirectory::MyRemoveDirectory(_diskFilePath);
      return S_OK;
    }

    if (!_isSplit)
    {
    NFile::NFind::CFileInfoW fileInfo;
    if(NFile::NFind::FindFile(fullProcessedPath, fileInfo))
    {
      switch(_overwriteMode)
      {
        case NExtract::NOverwriteMode::kSkipExisting:
          return S_OK;
        case NExtract::NOverwriteMode::kAskBefore:
        {
          Int32 overwiteResult;
          RINOK(_extractCallback2->AskOverwrite(
              fullProcessedPath, &fileInfo.LastWriteTime, &fileInfo.Size,
              fullPath, &_processedFileInfo.UTCLastWriteTime, newFileSizeDefined?
              &newFileSize : NULL, &overwiteResult))

          switch(overwiteResult)
          {
            case NOverwriteAnswer::kCancel:
              return E_ABORT;
            case NOverwriteAnswer::kNo:
              return S_OK;
            case NOverwriteAnswer::kNoToAll:
              _overwriteMode = NExtract::NOverwriteMode::kSkipExisting;
              return S_OK;
            case NOverwriteAnswer::kYesToAll:
              _overwriteMode = NExtract::NOverwriteMode::kWithoutPrompt;
              break;
            case NOverwriteAnswer::kYes:
              break;
            case NOverwriteAnswer::kAutoRename:
              _overwriteMode = NExtract::NOverwriteMode::kAutoRename;
              break;
            default:
              throw 20413;
          }
        }
      }
      if (_overwriteMode == NExtract::NOverwriteMode::kAutoRename)
      {
        if (!AutoRenamePath(fullProcessedPath))
        {
          UString message = UString(kCantAutoRename) + 
              fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_ABORT;
        }
      }
      else if (_overwriteMode == NExtract::NOverwriteMode::kAutoRenameExisting)
      {
        UString existPath = fullProcessedPath;
        if (!AutoRenamePath(existPath))
        {
          UString message = kCantAutoRename + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_ABORT;
        }
        if(!NFile::NDirectory::MyMoveFile(fullProcessedPath, existPath))
        {
          UString message = UString(kCantRenameFile) + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_ABORT;
        }
      }
      else
        if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath))
        {
          UString message = UString(kCantDeleteOutputFile) + 
              fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_ABORT;
        }
    }
    }
    if (!isAnti)
    {
      _outFileStreamSpec = new COutFileStream;
      CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
      if (!_outFileStreamSpec->File.Open(fullProcessedPath, 
          _isSplit ? OPEN_ALWAYS: CREATE_ALWAYS))
      {
        // if (::GetLastError() != ERROR_FILE_EXISTS || !isSplit)
        {
          UString message = L"can not open output file " + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return S_OK;
        }
      }
      if (_isSplit)
      {
        RINOK(_outFileStreamSpec->Seek(_position, STREAM_SEEK_SET, NULL));
      }
      _outFileStream = outStreamLoc;
      *outStream = outStreamLoc.Detach();
    }
    _diskFilePath = fullProcessedPath;
  }
  else
  {
    *outStream = NULL;
  }
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      _extractMode = true;
  };
  return _extractCallback2->PrepareOperation(_filePath, askExtractMode, _isSplit ? &_position: 0);
}

void CArchiveExtractCallback::AddErrorMessage(LPCTSTR message)
{
  _messages.Add(message);
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
    case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
    case NArchive::NExtract::NOperationResult::kCRCError:
    case NArchive::NExtract::NOperationResult::kDataError:
      break;
    default:
      _outFileStream.Release();
      return E_FAIL;
  }
  if(_outFileStream != NULL)
    _outFileStreamSpec->File.SetLastWriteTime(&_processedFileInfo.UTCLastWriteTime);
  _outFileStream.Release();
  if (_extractMode && _processedFileInfo.AttributesAreDefined)
    NFile::NDirectory::MySetFileAttributes(_diskFilePath, _processedFileInfo.Attributes);
  RINOK(_extractCallback2->SetOperationResult(operationResult));
  return S_OK;
}

/*
STDMETHODIMP CArchiveExtractCallback::GetInStream(
    const wchar_t *name, ISequentialInStream **inStream)
{
  CInFileStream *inFile = new CInFileStream;
  CMyComPtr<ISequentialInStream> inStreamTemp = inFile;
  if (!inFile->Open(_srcDirectoryPrefix + name))
    return ::GetLastError();
  *inStream = inStreamTemp.Detach();
  return S_OK;
}
*/

STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  if (!_cryptoGetTextPassword)
  {
    RINOK(_extractCallback2.QueryInterface(IID_ICryptoGetTextPassword, 
        &_cryptoGetTextPassword));
  }
  return _cryptoGetTextPassword->CryptoGetTextPassword(password);
}

