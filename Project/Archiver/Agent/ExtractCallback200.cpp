// ExtractCallback200.cpp

#include "StdAfx.h"

#include "ExtractCallback200.h"

#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/COM.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Time.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "Windows/PropVariantConversions.h"

#include "Util/FilePathAutoRename.h"

using namespace NWindows;

static const char *kTestingString    =  "Testing     ";
static const char *kExtractingString =  "Extracting  ";
static const char *kSkippingString   =  "Skipping    ";

extern void PrintMessage(const char *message);


CExtractCallBack200Imp::~CExtractCallBack200Imp()
{
}

void CExtractCallBack200Imp::Init(
    IArchiveHandler200 *archiveHandler,
    IExtractCallback2 *extractCallback2,
    const CSysString &directoryPath, 
    NExtractionMode::NPath::EEnum pathMode,
    NExtractionMode::NOverwrite::EEnum overwriteMode,
    const UStringVector &removePathParts,
    UINT codePage, 
    const UString &itemDefaultName,
    const FILETIME &utcLastWriteTimeDefault,
    UINT32 attributesDefault
    // bool aPasswordIsDefined,  const UString &aPassword
    )
{
  _extractCallback2 = extractCallback2;
  // m_PasswordIsDefined = aPasswordIsDefined;
  // m_Password = aPassword;
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
}

STDMETHODIMP CExtractCallBack200Imp::SetTotal(UINT64 aize)
{
  return _extractCallback2->SetTotal(aize);
}

STDMETHODIMP CExtractCallBack200Imp::SetCompleted(const UINT64 *completeValue)
{
  return _extractCallback2->SetCompleted(completeValue);
}

void CExtractCallBack200Imp::CreateComplexDirectory(const UStringVector &aDirPathParts)
{
  CSysString fullPath = _directoryPath;
  for(int i = 0; i < aDirPathParts.Size(); i++)
  {
    fullPath += GetSystemString(aDirPathParts[i], _codePage);
    NFile::NDirectory::MyCreateDirectory(fullPath);
    fullPath += NFile::NName::kDirDelimiter;
  }
}

static UString MakePathNameFromParts(const UStringVector &aParts)
{
  UString aResult;
  for(int i = 0; i < aParts.Size(); i++)
  {
    if(i != 0)
      aResult += wchar_t(NFile::NName::kDirDelimiter);
    aResult += aParts[i];
  }
  return aResult;
}


STDMETHODIMP CExtractCallBack200Imp::Extract(UINT32 anIndex, 
    ISequentialOutStream **outStream, INT32 askExtractMode)
{
  *outStream = 0;
  _outFileStream.Release();
  NCOM::CPropVariant aPropVariant;
  RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(anIndex, kpidPath, &aPropVariant));
  
  UString fullPath;
  if(aPropVariant.vt == VT_EMPTY)
  {
    fullPath = _itemDefaultName;
  }
  else 
  {
    if(aPropVariant.vt != VT_BSTR)
      return E_FAIL;
    fullPath = aPropVariant.bstrVal;
  }
  _filePath = fullPath;

  if(askExtractMode == NArchiveHandler::NExtract::NAskMode::kExtract)
  {
    RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(anIndex, kpidAttributes, &aPropVariant));
    if (aPropVariant.vt == VT_EMPTY)
    {
      _processedFileInfo.Attributes = _attributesDefault;
      _processedFileInfo.AttributesAreDefined = false;
    }
    else
    {
      if (aPropVariant.vt != VT_UI4)
        throw "incorrect item";
      _processedFileInfo.Attributes = aPropVariant.ulVal;
      _processedFileInfo.AttributesAreDefined = true;
    }

    RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(anIndex, kpidIsFolder, &aPropVariant));
    _processedFileInfo.IsDirectory = VARIANT_BOOLToBool(aPropVariant.boolVal);

    RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(anIndex, kpidLastWriteTime, &aPropVariant));
    switch(aPropVariant.vt)
    {
      case VT_EMPTY:
        _processedFileInfo.UTCLastWriteTime = _utcLastWriteTimeDefault;
        break;
      case VT_FILETIME:
        _processedFileInfo.UTCLastWriteTime = aPropVariant.filetime;
        break;
      default:
        return E_FAIL;
    }

    RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(anIndex, kpidSize, &aPropVariant));
    bool aNewFileSizeDefined = (aPropVariant.vt != VT_EMPTY);
    UINT64 aNewFileSize;
    if (aNewFileSizeDefined)
      aNewFileSize = ConvertPropVariantToUINT64(aPropVariant);

    bool isAnti = false;
    {
      NCOM::CPropVariant aPropVariantTemp;
      RETURN_IF_NOT_S_OK(_archiveHandler->GetProperty(anIndex, kpidIsAnti, 
          &aPropVariantTemp));
      if (aPropVariantTemp.vt != VT_EMPTY)
        isAnti = VARIANT_BOOLToBool(aPropVariantTemp.boolVal);
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


    UString aFullProcessedPathUnicode = 
        GetUnicodeString(_directoryPath, _codePage) + processedPath; 
    CSysString aFullProcessedPath = _directoryPath + 
        GetSystemString(processedPath, _codePage);

    if(_processedFileInfo.IsDirectory)
    {
      _diskFilePath = aFullProcessedPath;
      if (isAnti)
        ::RemoveDirectory(_diskFilePath);
      return S_OK;
    }

    NFile::NFind::CFileInfo fileInfo;
    if(NFile::NFind::FindFile(aFullProcessedPath, fileInfo))
    {
      switch(_overwriteMode)
      {
        case NExtractionMode::NOverwrite::kSkipExisting:
          return S_OK;
        case NExtractionMode::NOverwrite::kAskBefore:
        {
          INT32 aOverwiteResult;
          RETURN_IF_NOT_S_OK(_extractCallback2->AskOverwrite(
              aFullProcessedPathUnicode, &fileInfo.LastWriteTime, &fileInfo.Size,
              fullPath, &_processedFileInfo.UTCLastWriteTime, aNewFileSizeDefined?
              &aNewFileSize : NULL, &aOverwiteResult))

          switch(aOverwiteResult)
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
        if (!AutoRenamePath(aFullProcessedPath))
        {
          UString message = L"can not create name of file " + aFullProcessedPathUnicode;
          RETURN_IF_NOT_S_OK(_extractCallback2->MessageError(message));
          return E_ABORT;
        }
      }
      else
        if (!NFile::NDirectory::DeleteFileAlways(aFullProcessedPath))
        {
          UString message = L"can not delete output file " + aFullProcessedPathUnicode;
          RETURN_IF_NOT_S_OK(_extractCallback2->MessageError(message));
          return E_ABORT;
        }
    }
    if (!isAnti)
    {
      _outFileStreamSpec = new CComObjectNoLock<COutFileStream>;
      CComPtr<ISequentialOutStream> anOutStreamLoc(_outFileStreamSpec);
      if (!_outFileStreamSpec->Open(aFullProcessedPath))
      {
        UString message = L"can not open output file " + aFullProcessedPathUnicode;
        RETURN_IF_NOT_S_OK(_extractCallback2->MessageError(message));
        return S_OK;
      }
      _outFileStream = anOutStreamLoc;
      *outStream = anOutStreamLoc.Detach();
    }
    _diskFilePath = aFullProcessedPath;
  }
  else
  {
    *outStream = NULL;
  }
  return S_OK;
}

STDMETHODIMP CExtractCallBack200Imp::PrepareOperation(INT32 askExtractMode)
{
  _extractMode = false;
  switch (askExtractMode)
  {
    case NArchiveHandler::NExtract::NAskMode::kExtract:
      _extractMode = true;
  };
  return _extractCallback2->PrepareOperation(_filePath, askExtractMode);

  return S_OK;
}

void CExtractCallBack200Imp::AddErrorMessage(LPCTSTR message)
{
  _messages.Add(message);
}

STDMETHODIMP CExtractCallBack200Imp::OperationResult(INT32 anOperationResult)
{
  switch(anOperationResult)
  {
    case NArchiveHandler::NExtract::NOperationResult::kOK:
    case NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod:
    case NArchiveHandler::NExtract::NOperationResult::kCRCError:
    case NArchiveHandler::NExtract::NOperationResult::kDataError:
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
  RETURN_IF_NOT_S_OK(_extractCallback2->OperationResult(anOperationResult));
  return S_OK;
}

STDMETHODIMP CExtractCallBack200Imp::CryptoGetTextPassword(BSTR *aPassword)
{
  if (!_cryptoGetTextPassword)
  {
    RETURN_IF_NOT_S_OK(_extractCallback2.QueryInterface(&_cryptoGetTextPassword));
  }
  return _cryptoGetTextPassword->CryptoGetTextPassword(aPassword);
}

