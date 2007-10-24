// ArchiveExtractCallback.cpp

#include "StdAfx.h"

#include "ArchiveExtractCallback.h"

#include "Common/Wildcard.h"
#include "Common/StringConvert.h"
#include "Common/ComTry.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Time.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "Windows/PropVariantConversions.h"

#include "../../Common/FilePathAutoRename.h"

#include "../Common/ExtractingFilePath.h"
#include "OpenArchive.h"

using namespace NWindows;

static const wchar_t *kCantAutoRename = L"ERROR: Can not create file with auto name";
static const wchar_t *kCantRenameFile = L"ERROR: Can not rename existing file ";
static const wchar_t *kCantDeleteOutputFile = L"ERROR: Can not delete output file ";


void CArchiveExtractCallback::Init(
    IInArchive *archiveHandler,
    IFolderArchiveExtractCallback *extractCallback2,
    bool stdOutMode,
    const UString &directoryPath, 
    const UStringVector &removePathParts,
    const UString &itemDefaultName,
    const FILETIME &utcLastWriteTimeDefault,
    UInt32 attributesDefault,
    UInt64 packSize)
{
  _stdOutMode = stdOutMode;
  _numErrors = 0;
  _unpTotal = 1;
  _packTotal = packSize;

  _extractCallback2 = extractCallback2;
  _compressProgress.Release();
  _extractCallback2.QueryInterface(IID_ICompressProgressInfo, &_compressProgress);

  LocalProgressSpec->Init(extractCallback2, true);
  LocalProgressSpec->SendProgress = false;

  _itemDefaultName = itemDefaultName;
  _utcLastWriteTimeDefault = utcLastWriteTimeDefault;
  _attributesDefault = attributesDefault;
  _removePathParts = removePathParts;
  _archiveHandler = archiveHandler;
  _directoryPath = directoryPath;
  NFile::NName::NormalizeDirPathPrefix(_directoryPath);
}

STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 size)
{
  COM_TRY_BEGIN
  _unpTotal = size;
  if (!_multiArchives && _extractCallback2)
    return _extractCallback2->SetTotal(size);
  return S_OK;
  COM_TRY_END
}

static void NormalizeVals(UInt64 &v1, UInt64 &v2)
{
  const UInt64 kMax = (UInt64)1 << 31;
  while (v1 > kMax)
  {
    v1 >>= 1;
    v2 >>= 1;
  }
}

static UInt64 MyMultDiv64(UInt64 unpCur, UInt64 unpTotal, UInt64 packTotal)
{
  NormalizeVals(packTotal, unpTotal);
  NormalizeVals(unpCur, unpTotal);
  if (unpTotal == 0)
    unpTotal = 1;
  return unpCur * packTotal / unpTotal;
}

STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 *completeValue)
{
  COM_TRY_BEGIN
  if (!_extractCallback2)
    return S_OK;

  if (_multiArchives)
  {
    if (completeValue != NULL)
    {
      UInt64 packCur = LocalProgressSpec->InSize + MyMultDiv64(*completeValue, _unpTotal, _packTotal);
      return _extractCallback2->SetCompleted(&packCur);
    }
  }
  return _extractCallback2->SetCompleted(completeValue);
  COM_TRY_END
}

STDMETHODIMP CArchiveExtractCallback::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  COM_TRY_BEGIN
  return _localProgress->SetRatioInfo(inSize, outSize);
  COM_TRY_END
}

void CArchiveExtractCallback::CreateComplexDirectory(const UStringVector &dirPathParts, UString &fullPath)
{
  fullPath = _directoryPath;
  for(int i = 0; i < dirPathParts.Size(); i++)
  {
    if (i > 0)
      fullPath += wchar_t(NFile::NName::kDirDelimiter);
    fullPath += dirPathParts[i];
    NFile::NDirectory::MyCreateDirectory(fullPath);
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


HRESULT CArchiveExtractCallback::GetTime(int index, PROPID propID, FILETIME &filetime, bool &filetimeIsDefined)
{
  filetimeIsDefined = false;
  NCOM::CPropVariant prop;
  RINOK(_archiveHandler->GetProperty(index, propID, &prop));
  if (prop.vt == VT_FILETIME)
  {
    filetime = prop.filetime;
    filetimeIsDefined = (filetime.dwHighDateTime != 0 || filetime.dwLowDateTime != 0);
  }
  else if (prop.vt != VT_EMPTY)
    return E_FAIL;
  return S_OK;
}

STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode)
{
  COM_TRY_BEGIN
  *outStream = 0;
  _outFileStream.Release();

  _encrypted = false;
  _isSplit = false;
  _curSize = 0;

  UString fullPath;

  RINOK(GetArchiveItemPath(_archiveHandler, index, _itemDefaultName, fullPath));
  RINOK(IsArchiveItemFolder(_archiveHandler, index, _processedFileInfo.IsDirectory));

  _filePath = fullPath;

  {
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidPosition, &prop));
    if (prop.vt != VT_EMPTY)
    {
      if (prop.vt != VT_UI8)
        return E_FAIL;
      _position = prop.uhVal.QuadPart;
      _isSplit = true;
    }
  }
    
  RINOK(IsArchiveItemProp(_archiveHandler, index, kpidEncrypted, _encrypted));

  bool newFileSizeDefined;
  UInt64 newFileSize;
  {
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidSize, &prop));
    newFileSizeDefined = (prop.vt != VT_EMPTY);
    if (newFileSizeDefined)
    {
      newFileSize = ConvertPropVariantToUInt64(prop);
      _curSize = newFileSize;
    }
  }

  if(askExtractMode == NArchive::NExtract::NAskMode::kExtract)
  {
    if (_stdOutMode)
    {
      CMyComPtr<ISequentialOutStream> outStreamLoc = new CStdOutFileStream;
      *outStream = outStreamLoc.Detach();
      return S_OK;
    }

    {
      NCOM::CPropVariant prop;
      RINOK(_archiveHandler->GetProperty(index, kpidAttributes, &prop));
      if (prop.vt == VT_EMPTY)
      {
        _processedFileInfo.Attributes = _attributesDefault;
        _processedFileInfo.AttributesAreDefined = false;
      }
      else
      {
        if (prop.vt != VT_UI4)
          return E_FAIL;
        _processedFileInfo.Attributes = prop.ulVal;
        _processedFileInfo.AttributesAreDefined = true;
      }
    }

    RINOK(GetTime(index, kpidCreationTime, _processedFileInfo.CreationTime,
        _processedFileInfo.IsCreationTimeDefined));
    RINOK(GetTime(index, kpidLastWriteTime, _processedFileInfo.LastWriteTime, 
        _processedFileInfo.IsLastWriteTimeDefined));
    RINOK(GetTime(index, kpidLastAccessTime, _processedFileInfo.LastAccessTime,
        _processedFileInfo.IsLastAccessTimeDefined));

    bool isAnti = false;
    RINOK(IsArchiveItemProp(_archiveHandler, index, kpidIsAnti, isAnti));

    UStringVector pathParts; 
    SplitPathToParts(fullPath, pathParts);
    
    if(pathParts.IsEmpty())
      return E_FAIL;
    int numRemovePathParts = 0;
    switch(_pathMode)
    {
      case NExtract::NPathMode::kFullPathnames:
        break;
      case NExtract::NPathMode::kCurrentPathnames:
      {
        numRemovePathParts = _removePathParts.Size();
        if (pathParts.Size() <= numRemovePathParts)
          return E_FAIL;
        for (int i = 0; i < numRemovePathParts; i++)
          if (_removePathParts[i].CompareNoCase(pathParts[i]) != 0)
            return E_FAIL;
        break;
      }
      case NExtract::NPathMode::kNoPathnames:
      {
        numRemovePathParts = pathParts.Size() - 1;
        break;
      }
    }
    pathParts.Delete(0, numRemovePathParts);
    MakeCorrectPath(pathParts);
    UString processedPath = MakePathNameFromParts(pathParts);
    if (!isAnti)
    {
      if (!_processedFileInfo.IsDirectory)
      {
        if (!pathParts.IsEmpty())
          pathParts.DeleteBack();
      }
    
      if (!pathParts.IsEmpty())
      {
        UString fullPathNew;
        CreateComplexDirectory(pathParts, fullPathNew);
        if (_processedFileInfo.IsDirectory)
          NFile::NDirectory::SetDirTime(fullPathNew, 
            (WriteCreated && _processedFileInfo.IsCreationTimeDefined) ? &_processedFileInfo.CreationTime : NULL, 
            (WriteAccessed && _processedFileInfo.IsLastAccessTimeDefined) ? &_processedFileInfo.LastAccessTime : NULL, 
            (WriteModified && _processedFileInfo.IsLastWriteTimeDefined) ? &_processedFileInfo.LastWriteTime : &_utcLastWriteTimeDefault);
      }
    }


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
              fullProcessedPath, &fileInfo.LastWriteTime, &fileInfo.Size, fullPath, 
              _processedFileInfo.IsLastWriteTimeDefined ? &_processedFileInfo.LastWriteTime : NULL, 
              newFileSizeDefined ? &newFileSize : NULL, 
              &overwiteResult))

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
              return E_FAIL;
          }
        }
      }
      if (_overwriteMode == NExtract::NOverwriteMode::kAutoRename)
      {
        if (!AutoRenamePath(fullProcessedPath))
        {
          UString message = UString(kCantAutoRename) + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_FAIL;
        }
      }
      else if (_overwriteMode == NExtract::NOverwriteMode::kAutoRenameExisting)
      {
        UString existPath = fullProcessedPath;
        if (!AutoRenamePath(existPath))
        {
          UString message = kCantAutoRename + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_FAIL;
        }
        if(!NFile::NDirectory::MyMoveFile(fullProcessedPath, existPath))
        {
          UString message = UString(kCantRenameFile) + fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return E_FAIL;
        }
      }
      else
        if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath))
        {
          UString message = UString(kCantDeleteOutputFile) +  fullProcessedPath;
          RINOK(_extractCallback2->MessageError(message));
          return S_OK;
          // return E_FAIL;
        }
    }
    }
    if (!isAnti)
    {
      _outFileStreamSpec = new COutFileStream;
      CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
      if (!_outFileStreamSpec->Open(fullProcessedPath, _isSplit ? OPEN_ALWAYS: CREATE_ALWAYS))
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
  COM_TRY_END
}

STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
{
  COM_TRY_BEGIN
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      _extractMode = true;
  };
  return _extractCallback2->PrepareOperation(_filePath, _processedFileInfo.IsDirectory, 
      askExtractMode, _isSplit ? &_position: 0);
  COM_TRY_END
}

STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
{
  COM_TRY_BEGIN
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
  if (_outFileStream != NULL)
  {
    _outFileStreamSpec->SetTime(
        (WriteCreated && _processedFileInfo.IsCreationTimeDefined) ? &_processedFileInfo.CreationTime : NULL, 
        (WriteAccessed && _processedFileInfo.IsLastAccessTimeDefined) ? &_processedFileInfo.LastAccessTime : NULL, 
        (WriteModified && _processedFileInfo.IsLastWriteTimeDefined) ? &_processedFileInfo.LastWriteTime : &_utcLastWriteTimeDefault);
    _curSize = _outFileStreamSpec->ProcessedSize;
    RINOK(_outFileStreamSpec->Close());
    _outFileStream.Release();
  }
  UnpackSize += _curSize;
  if (_processedFileInfo.IsDirectory)
    NumFolders++;
  else
    NumFiles++;

  if (_extractMode && _processedFileInfo.AttributesAreDefined)
    NFile::NDirectory::MySetFileAttributes(_diskFilePath, _processedFileInfo.Attributes);
  RINOK(_extractCallback2->SetOperationResult(operationResult, _encrypted));
  return S_OK;
  COM_TRY_END
}

/*
STDMETHODIMP CArchiveExtractCallback::GetInStream(
    const wchar_t *name, ISequentialInStream **inStream)
{
  COM_TRY_BEGIN
  CInFileStream *inFile = new CInFileStream;
  CMyComPtr<ISequentialInStream> inStreamTemp = inFile;
  if (!inFile->Open(_srcDirectoryPrefix + name))
    return ::GetLastError();
  *inStream = inStreamTemp.Detach();
  return S_OK;
  COM_TRY_END
}
*/

STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
{
  COM_TRY_BEGIN
  if (!_cryptoGetTextPassword)
  {
    RINOK(_extractCallback2.QueryInterface(IID_ICryptoGetTextPassword, 
        &_cryptoGetTextPassword));
  }
  return _cryptoGetTextPassword->CryptoGetTextPassword(password);
  COM_TRY_END
}

