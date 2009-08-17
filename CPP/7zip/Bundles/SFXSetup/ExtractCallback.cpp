// ExtractCallback.h

#include "StdAfx.h"

#include "Common/Wildcard.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "ExtractCallback.h"

using namespace NWindows;
using namespace NFile;

static LPCWSTR kCantDeleteFile = L"Can not delete output file";
static LPCWSTR kCantOpenFile = L"Can not open output file";
static LPCWSTR kUnsupportedMethod = L"Unsupported Method";

void CExtractCallbackImp::Init(IInArchive *archiveHandler,
    const UString &directoryPath,
    const UString &itemDefaultName,
    const FILETIME &defaultMTime,
    UInt32 defaultAttributes)
{
  _message.Empty();
  _isCorrupt = false;
  _itemDefaultName = itemDefaultName;
  _defaultMTime = defaultMTime;
  _defaultAttributes = defaultAttributes;
  _archiveHandler = archiveHandler;
  _directoryPath = directoryPath;
  NName::NormalizeDirPathPrefix(_directoryPath);
}

HRESULT CExtractCallbackImp::Open_CheckBreak()
{
  #ifndef _NO_PROGRESS
  return ProgressDialog.Sync.ProcessStopAndPause();
  #else
  return S_OK;
  #endif
}

HRESULT CExtractCallbackImp::Open_SetTotal(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  return S_OK;
}

HRESULT CExtractCallbackImp::Open_SetCompleted(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  #ifndef _NO_PROGRESS
  return ProgressDialog.Sync.ProcessStopAndPause();
  #else
  return S_OK;
  #endif
}

STDMETHODIMP CExtractCallbackImp::SetTotal(UInt64 size)
{
  #ifndef _NO_PROGRESS
  ProgressDialog.Sync.SetProgress(size, 0);
  #endif
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompleted(const UInt64 *completeValue)
{
  #ifndef _NO_PROGRESS
  RINOK(ProgressDialog.Sync.ProcessStopAndPause());
  if (completeValue != NULL)
    ProgressDialog.Sync.SetPos(*completeValue);
  #endif
  return S_OK;
}

void CExtractCallbackImp::CreateComplexDirectory(const UStringVector &dirPathParts)
{
  UString fullPath = _directoryPath;
  for(int i = 0; i < dirPathParts.Size(); i++)
  {
    fullPath += dirPathParts[i];
    NDirectory::MyCreateDirectory(fullPath);
    fullPath += NName::kDirDelimiter;
  }
}

STDMETHODIMP CExtractCallbackImp::GetStream(UInt32 index,
    ISequentialOutStream **outStream, Int32 askExtractMode)
{
  #ifndef _NO_PROGRESS
  if (ProgressDialog.Sync.GetStopped())
    return E_ABORT;
  #endif
  _outFileStream.Release();
  NCOM::CPropVariant propVariantName;
  RINOK(_archiveHandler->GetProperty(index, kpidPath, &propVariantName));
  UString fullPath;
  if (propVariantName.vt == VT_EMPTY)
    fullPath = _itemDefaultName;
  else
  {
    if (propVariantName.vt != VT_BSTR)
      return E_FAIL;
    fullPath = propVariantName.bstrVal;
  }
  _filePath = fullPath;

  if (askExtractMode == NArchive::NExtract::NAskMode::kExtract)
  {
    NCOM::CPropVariant prop;
    RINOK(_archiveHandler->GetProperty(index, kpidAttrib, &prop));
    if (prop.vt == VT_EMPTY)
      _processedFileInfo.Attributes = _defaultAttributes;
    else
    {
      if (prop.vt != VT_UI4)
        return E_FAIL;
      _processedFileInfo.Attributes = prop.ulVal;
    }

    RINOK(_archiveHandler->GetProperty(index, kpidIsDir, &prop));
    _processedFileInfo.IsDir = VARIANT_BOOLToBool(prop.boolVal);

    bool isAnti = false;
    {
      NCOM::CPropVariant propTemp;
      RINOK(_archiveHandler->GetProperty(index, kpidIsAnti, &propTemp));
      if (propTemp.vt == VT_BOOL)
        isAnti = VARIANT_BOOLToBool(propTemp.boolVal);
    }

    RINOK(_archiveHandler->GetProperty(index, kpidMTime, &prop));
    switch(prop.vt)
    {
      case VT_EMPTY: _processedFileInfo.MTime = _defaultMTime; break;
      case VT_FILETIME: _processedFileInfo.MTime = prop.filetime; break;
      default: return E_FAIL;
    }

    UStringVector pathParts;
    SplitPathToParts(fullPath, pathParts);
    if (pathParts.IsEmpty())
      return E_FAIL;

    UString processedPath = fullPath;

    if (!_processedFileInfo.IsDir)
      pathParts.DeleteBack();
    if (!pathParts.IsEmpty())
    {
      if (!isAnti)
        CreateComplexDirectory(pathParts);
    }

    UString fullProcessedPath = _directoryPath + processedPath;

    if (_processedFileInfo.IsDir)
    {
      _diskFilePath = fullProcessedPath;

      if (isAnti)
        NDirectory::MyRemoveDirectory(_diskFilePath);
      else
        NDirectory::SetDirTime(_diskFilePath, NULL, NULL, &_processedFileInfo.MTime);
      return S_OK;
    }

    NFind::CFileInfoW fileInfo;
    if (fileInfo.Find(fullProcessedPath))
    {
      if (!NDirectory::DeleteFileAlways(fullProcessedPath))
      {
        _message = kCantDeleteFile;
        return E_FAIL;
      }
    }

    if (!isAnti)
    {
      _outFileStreamSpec = new COutFileStream;
      CMyComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
      if (!_outFileStreamSpec->Create(fullProcessedPath, true))
      {
        _message = kCantOpenFile;
        return E_FAIL;
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

STDMETHODIMP CExtractCallbackImp::PrepareOperation(Int32 askExtractMode)
{
  _extractMode = (askExtractMode == NArchive::NExtract::NAskMode::kExtract);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetOperationResult(Int32 resultEOperationResult)
{
  switch(resultEOperationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;

    default:
    {
      _outFileStream.Release();
      switch(resultEOperationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          _message = kUnsupportedMethod;
          break;
        default:
          _isCorrupt = true;
      }
      return E_FAIL;
    }
  }
  if (_outFileStream != NULL)
  {
    _outFileStreamSpec->SetMTime(&_processedFileInfo.MTime);
    RINOK(_outFileStreamSpec->Close());
  }
  _outFileStream.Release();
  if (_extractMode)
    NDirectory::MySetFileAttributes(_diskFilePath, _processedFileInfo.Attributes);
  return S_OK;
}
