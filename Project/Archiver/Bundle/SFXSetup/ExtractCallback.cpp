// ExtractCallback.h

#include "StdAfx.h"

#include "ExtractCallback.h"

#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/COM.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Time.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "Windows/PropVariantConversions.h"

using namespace NWindows;
using namespace NZipSettings;

CExtractCallbackImp::~CExtractCallbackImp()
{
  _progressDialog.Destroy();
}

void CExtractCallbackImp::Init(IInArchive *archiveHandler,
    const CSysString &directoryPath,   
    const UString &itemDefaultName,
    const FILETIME &utcLastWriteTimeDefault,
    UINT32 attributesDefault)
{
  _codePage = CP_ACP;
  _numErrors = 0;
  _itemDefaultName = itemDefaultName;
  _utcLastWriteTimeDefault = utcLastWriteTimeDefault;
  _attributesDefault = attributesDefault;
  _archiveHandler = archiveHandler;
  _directoryPath = directoryPath;
  NFile::NName::NormalizeDirPathPrefix(_directoryPath);
}

STDMETHODIMP CExtractCallbackImp::SetTotal(UINT64 size)
{
  /*
  if (_threadID != GetCurrentThreadId())
    return S_OK;
  */
  _progressDialog._progressSynch.SetProgress(size, 0);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompleted(const UINT64 *completeValue)
{
  /*
  if (_threadID != GetCurrentThreadId())
    return S_OK;

  ProcessMessages(_progressDialog);
  */
  if(_progressDialog._progressSynch.GetStopped())
    return E_ABORT;
  if (completeValue != NULL)
    _progressDialog._progressSynch.SetPos(*completeValue);
  return S_OK;
}

void CExtractCallbackImp::CreateComplexDirectory(const UStringVector &dirPathParts)
{
  CSysString fullPath = _directoryPath;
  for(int i = 0; i < dirPathParts.Size(); i++)
  {
    fullPath += GetSystemString(dirPathParts[i], _codePage);
    NFile::NDirectory::MyCreateDirectory(fullPath);
    fullPath += NFile::NName::kDirDelimiter;
  }
}

STDMETHODIMP CExtractCallbackImp::GetStream(UINT32 index,
    ISequentialOutStream **outStream, INT32 askExtractMode)
{
  // ProcessMessages(_progressDialog);
  if(_progressDialog._progressSynch.GetStopped())
    return E_ABORT;
  _outFileStream.Release();
  NCOM::CPropVariant propVariantName;
  RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(index, kpidPath, &propVariantName));
  UString fullPath;
  if(propVariantName.vt == VT_EMPTY)
    fullPath = _itemDefaultName;
  else 
  {
    if(propVariantName.vt != VT_BSTR)
      return E_FAIL;
    fullPath = propVariantName.bstrVal;
  }
  _filePath = GetSystemString(fullPath, _codePage);

  // m_CurrentFilePath = GetSystemString(fullPath, _codePage);
  
  if(askExtractMode == NArchive::NExtract::NAskMode::kExtract)
  {
    NCOM::CPropVariant propVariant;
    RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(index, kpidAttributes, &propVariant));
    if (propVariant.vt == VT_EMPTY)
      _processedFileInfo.Attributes = _attributesDefault;
    else
    {
      if (propVariant.vt != VT_UI4)
        return E_FAIL;
      _processedFileInfo.Attributes = propVariant.ulVal;
    }

    RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(index, kpidIsFolder, &propVariant));
    _processedFileInfo.IsDirectory = VARIANT_BOOLToBool(propVariant.boolVal);

    bool isAnti = false;
    {
      NCOM::CPropVariant propVariantTemp;
      RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(index, kpidIsAnti, 
          &propVariantTemp));
      if (propVariantTemp.vt == VT_BOOL)
        isAnti = VARIANT_BOOLToBool(propVariantTemp.boolVal);
    }

    RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(index, kpidLastWriteTime, &propVariant));
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

    UStringVector pathParts; 
    SplitPathToParts(fullPath, pathParts);
    if(pathParts.IsEmpty())
      return E_FAIL;

    UString processedPath = fullPath;

    if(!_processedFileInfo.IsDirectory)
      pathParts.DeleteBack();
    if (!pathParts.IsEmpty())
    {
      if (!isAnti)
        CreateComplexDirectory(pathParts);
    }

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
      if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath))
      {
        MessageBox(0, "Can not delete output file", "7-Zip", 0);
        // g_StdOut << GetOemString(fullProcessedPath);
        // return E_ABORT;
        return E_ABORT;
      }
    }

    if (!isAnti)
    {
      _outFileStreamSpec = new CComObjectNoLock<COutFileStream>;
      CComPtr<ISequentialOutStream> outStreamLoc(_outFileStreamSpec);
      if (!_outFileStreamSpec->Open(fullProcessedPath))
      {
        MessageBox(0, "Can not open output file", "7-Zip", 0);
        // g_StdOut << GetOemString(fullProcessedPath);
        return E_ABORT;
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

STDMETHODIMP CExtractCallbackImp::PrepareOperation(INT32 askExtractMode)
{
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      _extractMode = true;
      break;
  };
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetOperationResult(INT32 resultEOperationResult)
{
  switch(resultEOperationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
    {
      break;
    }
    default:
    {
      _numErrors++;
      switch(resultEOperationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          MessageBox(0, "Unsupported Method", "7-Zip", 0);
          return E_FAIL;
          // break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          MessageBox(0, "CRC Failed", "7-Zip", 0);
          return E_FAIL;
          // break;
        case NArchive::NExtract::NOperationResult::kDataError:
          MessageBox(0, "Data Error", "7-Zip", 0);
          return E_FAIL;
          // break;
        default:
          _outFileStream.Release();
          return E_FAIL;
      }
    }
  }
  if(_outFileStream != NULL)
    _outFileStreamSpec->File.SetLastWriteTime(&_processedFileInfo.UTCLastWriteTime);
  _outFileStream.Release();
  if (_extractMode)
    SetFileAttributes(_diskFilePath, _processedFileInfo.Attributes);
  return S_OK;
}

 
