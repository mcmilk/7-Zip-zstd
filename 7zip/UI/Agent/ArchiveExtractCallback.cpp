// ExtractCallback.cpp

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

using namespace NWindows;

void CArchiveExtractCallback::Init(
    IInArchive *archiveHandler,
    IFolderArchiveExtractCallback *extractCallback2,
    const CSysString &directoryPath, 
    NExtractionMode::NPath::EEnum pathMode,
    NExtractionMode::NOverwrite::EEnum overwriteMode,
    const UStringVector &removePathParts,
    UINT codePage, 
    const UString &itemDefaultName,
    const FILETIME &utcLastWriteTimeDefault,
    UINT32 attributesDefault)
    // bool passwordIsDefined, const UString &password
    // CSysString srcDirectoryPrefix)
{
  _extractCallback2 = extractCallback2;
  // m_PasswordIsDefined = passwordIsDefined;
  // m_Password = password;
  _numErrors = 0;

  _itemDefaultName = itemDefaultName;
  _utcLastWriteTimeDefault = utcLastWriteTimeDefault;
  _attributesDefault = attributesDefault;
  
  _codePage = codePage;
  _removePathParts = removePathParts;

  _pathMode = pathMode;
  _overwriteMode = overwriteMode;

  _archiveHandler = archiveHandler;
  _directoryPath = directoryPath;
  NFile::NName::NormalizeDirPathPrefix(_directoryPath);

  // _srcDirectoryPrefix = srcDirectoryPrefix;
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UINT64 size)
{
  return _extractCallback2->SetTotal(size);
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UINT64 *completeValue)
{
  return _extractCallback2->SetCompleted(completeValue);
}

void CArchiveExtractCallback::CreateComplexDirectory(const UStringVector &dirPathParts)
{
  CSysString fullPath = _directoryPath;
  for(int i = 0; i < dirPathParts.Size(); i++)
  {
    fullPath += GetSystemString(dirPathParts[i], _codePage);
    NFile::NDirectory::MyCreateDirectory(fullPath);
    fullPath += NFile::NName::kDirDelimiter;
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


STDMETHODIMP CArchiveExtractCallback::GetStream(UINT32 index, 
    ISequentialOutStream **outStream, INT32 askExtractMode)
{
  *outStream = 0;
  _outFileStream.Release();
  NCOM::CPropVariant propVariant;
  RINOK(_archiveHandler->GetProperty(index, kpidPath, &propVariant));
  
  UString fullPath;
  if(propVariant.vt == VT_EMPTY)
  {
    fullPath = _itemDefaultName;
  }
  else 
  {
    if(propVariant.vt != VT_BSTR)
      return E_FAIL;
    fullPath = propVariant.bstrVal;
  }
  _filePath = fullPath;

  if(askExtractMode == NArchive::NExtract::NAskMode::kExtract)
  {
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
    UINT64 newFileSize;
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
    SplitPathToParts(fullPath, pathParts);
    if(pathParts.IsEmpty())
      return E_FAIL;
    UString processedPath;
    switch(_pathMode)
    {
      case NExtractionMode::NPath::kFullPathnames:
      {
        processedPath = fullPath;
        break;
      }
      case NExtractionMode::NPath::kCurrentPathnames:
      {
        int numRemovePathParts = _removePathParts.Size();
        if(pathParts.Size() <= numRemovePathParts)
          return E_FAIL;
        for(int i = 0; i < numRemovePathParts; i++)
          if(_removePathParts[i].CollateNoCase(pathParts[i]) != 0)
            return E_FAIL;
        pathParts.Delete(0, numRemovePathParts);
        processedPath = MakePathNameFromParts(pathParts);
        break;
      }
      case NExtractionMode::NPath::kNoPathnames:
      {
        processedPath = pathParts.Back(); 
        pathParts.Delete(0, pathParts.Size() - 1); // Test it!!
        break;
      }
    }
    if(!_processedFileInfo.IsDirectory)
      pathParts.DeleteBack();
    
    if (!isAnti)
      if (!pathParts.IsEmpty())
        CreateComplexDirectory(pathParts);


    UString fullProcessedPathUnicode = 
        GetUnicodeString(_directoryPath, _codePage) + processedPath; 
    CSysString fullProcessedPath = _directoryPath + 
        GetSystemString(processedPath, _codePage);

    if(_processedFileInfo.IsDirectory)
    {
      _diskFilePath = fullProcessedPath;
      if (isAnti)
        ::RemoveDirectory(_diskFilePath);
      return S_OK;
    }

    NFile::NFind::CFileInfo fileInfo;
    if(NFile::NFind::FindFile(fullProcessedPath, fileInfo))
    {
      switch(_overwriteMode)
      {
        case NExtractionMode::NOverwrite::kSkipExisting:
          return S_OK;
        case NExtractionMode::NOverwrite::kAskBefore:
        {
          INT32 overwiteResult;
          RINOK(_extractCallback2->AskOverwrite(
              fullProcessedPathUnicode, &fileInfo.LastWriteTime, &fileInfo.Size,
              fullPath, &_processedFileInfo.UTCLastWriteTime, newFileSizeDefined?
              &newFileSize : NULL, &overwiteResult))

          switch(overwiteResult)
          {
            case NOverwriteAnswer::kCancel:
              return E_ABORT;
            case NOverwriteAnswer::kNo:
              return S_OK;
            case NOverwriteAnswer::kNoToAll:
              _overwriteMode = NExtractionMode::NOverwrite::kSkipExisting;
              return S_OK;
            case NOverwriteAnswer::kYesToAll:
              _overwriteMode = NExtractionMode::NOverwrite::kWithoutPrompt;
              break;
            case NOverwriteAnswer::kYes:
              break;
            case NOverwriteAnswer::kAutoRename:
              _overwriteMode = NExtractionMode::NOverwrite::kAutoRename;
              break;
            default:
              throw 20413;
          }
        }
      }
      if (_overwriteMode == NExtractionMode::NOverwrite::kAutoRename)
      {
        if (!AutoRenamePath(fullProcessedPath))
        {
          UString message = L"can not create name of file " + fullProcessedPathUnicode;
          RINOK(_extractCallback2->MessageError(message));
          return E_ABORT;
        }
      }
      else
        if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath))
        {
          UString message = L"can not delete output file " + fullProcessedPathUnicode;
          RINOK(_extractCallback2->MessageError(message));
          return E_ABORT;
        }
    }
    if (!isAnti)
    {
      _outFileStreamSpec = new COutFileStream;
      CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
      if (!_outFileStreamSpec->Open(fullProcessedPath))
      {
        UString message = L"can not open output file " + fullProcessedPathUnicode;
        RINOK(_extractCallback2->MessageError(message));
        return S_OK;
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

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(INT32 askExtractMode)
{
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      _extractMode = true;
  };
  return _extractCallback2->PrepareOperation(_filePath, askExtractMode);

  // return S_OK;
}

void CArchiveExtractCallback::AddErrorMessage(LPCTSTR message)
{
  _messages.Add(message);
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(INT32 operationResult)
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
    SetFileAttributes(_diskFilePath, _processedFileInfo.Attributes);
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

