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

#include "../Common/ExtractAutoRename.h"

using namespace NWindows;

static const char *kTestingString    =  "Testing     ";
static const char *kExtractingString =  "Extracting  ";
static const char *kSkippingString   =  "Skipping    ";

extern void PrintMessage(const char *aMessage);


CExtractCallBack200Imp::~CExtractCallBack200Imp()
{
}

void CExtractCallBack200Imp::Init(
    IArchiveHandler200 *anArchiveHandler,
    IExtractCallback2 *anExtractCallback2,
    const CSysString &aDirectoryPath, 
    NExtractionMode::NPath::EEnum aPathMode,
    NExtractionMode::NOverwrite::EEnum anOverwriteMode,
    const UStringVector &aRemovePathParts,
    UINT aCodePage, 
    const UString &anItemDefaultName,
    const FILETIME &anUTCLastWriteTimeDefault,
    UINT32 anAttributesDefault,
    bool aPasswordIsDefined, 
    const UString &aPassword)
{
  m_ExtractCallback2 = anExtractCallback2;
  m_PasswordIsDefined = aPasswordIsDefined;
  m_Password = aPassword;
  m_NumErrors = 0;

  m_ItemDefaultName = anItemDefaultName;
  m_UTCLastWriteTimeDefault = anUTCLastWriteTimeDefault;
  m_AttributesDefault = anAttributesDefault;
  
  m_CodePage = aCodePage;
  m_RemovePathParts = aRemovePathParts;

  m_PathMode = aPathMode;
  m_OverwriteMode = anOverwriteMode;

  m_ArchiveHandler = anArchiveHandler;
  m_DirectoryPath = aDirectoryPath;
  NFile::NName::NormalizeDirPathPrefix(m_DirectoryPath);
}

STDMETHODIMP CExtractCallBack200Imp::SetTotal(UINT64 aSize)
{
  return m_ExtractCallback2->SetTotal(aSize);
}

STDMETHODIMP CExtractCallBack200Imp::SetCompleted(const UINT64 *aCompleteValue)
{
  return m_ExtractCallback2->SetCompleted(aCompleteValue);
}

void CExtractCallBack200Imp::CreateComplexDirectory(const UStringVector &aDirPathParts)
{
  CSysString aFullPath = m_DirectoryPath;
  for(int i = 0; i < aDirPathParts.Size(); i++)
  {
    aFullPath += GetSystemString(aDirPathParts[i], m_CodePage);
    NFile::NDirectory::MyCreateDirectory(aFullPath);
    aFullPath += NFile::NName::kDirDelimiter;
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
    ISequentialOutStream **anOutStream, INT32 anAskExtractMode)
{
  m_OutFileStream.Release();
  NCOM::CPropVariant aPropVariant;
  RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidPath, &aPropVariant));
  
  UString aFullPath;
  if(aPropVariant.vt == VT_EMPTY)
  {
    aFullPath = m_ItemDefaultName;
  }
  else 
  {
    if(aPropVariant.vt != VT_BSTR)
      return E_FAIL;
    aFullPath = aPropVariant.bstrVal;
  }
  m_FilePath = aFullPath;

  if(anAskExtractMode == NArchiveHandler::NExtract::NAskMode::kExtract)
  {
    RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidAttributes, &aPropVariant));
    if (aPropVariant.vt == VT_EMPTY)
      m_ProcessedFileInfo.Attributes = m_AttributesDefault;
    else
    {
      if (aPropVariant.vt != VT_UI4)
        throw "incorrect item";
      m_ProcessedFileInfo.Attributes = aPropVariant.ulVal;
    }

    RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidIsFolder, &aPropVariant));
    m_ProcessedFileInfo.IsDirectory = VARIANT_BOOLToBool(aPropVariant.boolVal);

    RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidLastWriteTime, &aPropVariant));
    switch(aPropVariant.vt)
    {
      case VT_EMPTY:
        m_ProcessedFileInfo.UTCLastWriteTime = m_UTCLastWriteTimeDefault;
        break;
      case VT_FILETIME:
        m_ProcessedFileInfo.UTCLastWriteTime = aPropVariant.filetime;
        break;
      default:
        return E_FAIL;
    }

    RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidSize, &aPropVariant));
    bool aNewFileSizeDefined = (aPropVariant.vt != VT_EMPTY);
    UINT64 aNewFileSize;
    if (aNewFileSizeDefined)
      aNewFileSize = ConvertPropVariantToUINT64(aPropVariant);

    bool anIsAnti = false;
    {
      NCOM::CPropVariant aPropVariantTemp;
      RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidIsAnti, 
          &aPropVariantTemp));
      if (aPropVariantTemp.vt != VT_EMPTY)
        anIsAnti = VARIANT_BOOLToBool(aPropVariantTemp.boolVal);
    }

    UStringVector aPathParts; 
    SplitPathToParts(aFullPath, aPathParts);
    if(aPathParts.IsEmpty())
      return E_FAIL;
    UString aProcessedPath;
    switch(m_PathMode)
    {
      case NExtractionMode::NPath::kFullPathnames:
      {
        aProcessedPath = aFullPath;
        break;
      }
      case NExtractionMode::NPath::kCurrentPathnames:
      {
        int aNumRemovePathParts = m_RemovePathParts.Size();
        if(aPathParts.Size() <= aNumRemovePathParts)
          return E_FAIL;
        for(int i = 0; i < aNumRemovePathParts; i++)
          if(m_RemovePathParts[i].CollateNoCase(aPathParts[i]) != 0)
            return E_FAIL;
        aPathParts.Delete(0, aNumRemovePathParts);
        aProcessedPath = MakePathNameFromParts(aPathParts);
        break;
      }
      case NExtractionMode::NPath::kNoPathnames:
      {
        aProcessedPath = aPathParts.Back(); 
        aPathParts.Delete(0, aPathParts.Size() - 1); // Test it!!
        break;
      }
    }
    if(!m_ProcessedFileInfo.IsDirectory)
      aPathParts.DeleteBack();
    
    if (!anIsAnti)
      if (!aPathParts.IsEmpty())
        CreateComplexDirectory(aPathParts);


    UString aFullProcessedPathUnicode = 
        GetUnicodeString(m_DirectoryPath, m_CodePage) + aProcessedPath; 
    CSysString aFullProcessedPath = m_DirectoryPath + 
        GetSystemString(aProcessedPath, m_CodePage);

    if(m_ProcessedFileInfo.IsDirectory)
    {
      m_DiskFilePath = aFullProcessedPath;
      if (anIsAnti)
        ::RemoveDirectory(m_DiskFilePath);
      return S_OK;
    }

    NFile::NFind::CFileInfo aFileInfo;
    if(NFile::NFind::FindFile(aFullProcessedPath, aFileInfo))
    {
      switch(m_OverwriteMode)
      {
        case NExtractionMode::NOverwrite::kSkipExisting:
          return S_OK;
        case NExtractionMode::NOverwrite::kAskBefore:
        {
          INT32 aOverwiteResult;
          RETURN_IF_NOT_S_OK(m_ExtractCallback2->AskOverwrite(
              aFullProcessedPathUnicode, &aFileInfo.LastWriteTime, &aFileInfo.Size,
              aFullPath, &m_ProcessedFileInfo.UTCLastWriteTime, aNewFileSizeDefined?
              &aNewFileSize : NULL, &aOverwiteResult))

          switch(aOverwiteResult)
          {
            case NOverwriteAnswer::kCancel:
              return E_ABORT;
            case NOverwriteAnswer::kNo:
              return S_OK;
            case NOverwriteAnswer::kNoToAll:
              m_OverwriteMode = NExtractionMode::NOverwrite::kSkipExisting;
              return S_OK;
            case NOverwriteAnswer::kYesToAll:
              m_OverwriteMode = NExtractionMode::NOverwrite::kWithoutPrompt;
              break;
            case NOverwriteAnswer::kYes:
              break;
            case NOverwriteAnswer::kAutoRename:
              m_OverwriteMode = NExtractionMode::NOverwrite::kAutoRename;
              break;
            default:
              throw 20413;
          }
        }
      }
      if (m_OverwriteMode == NExtractionMode::NOverwrite::kAutoRename)
      {
        if (!AutoRenamePath(aFullProcessedPath))
        {
          RETURN_IF_NOT_S_OK(m_ExtractCallback2->MessageError(
            L"can not create name of file"));
          return E_ABORT;
        }
      }
      else
        if (!NFile::NDirectory::DeleteFileAlways(aFullProcessedPath))
        {
          RETURN_IF_NOT_S_OK(m_ExtractCallback2->MessageError(
              L"can not delete output file "));
          return E_ABORT;
        }
    }
    if (!anIsAnti)
    {
      m_OutFileStreamSpec = new CComObjectNoLock<COutFileStream>;
      CComPtr<ISequentialOutStream> anOutStreamLoc(m_OutFileStreamSpec);
      if (!m_OutFileStreamSpec->Open(aFullProcessedPath))
      {
        RETURN_IF_NOT_S_OK(m_ExtractCallback2->MessageError(
            L"can not open output file "));
        return E_ABORT;
      }
      m_OutFileStream = anOutStreamLoc;
      *anOutStream = anOutStreamLoc.Detach();
    }
    m_DiskFilePath = aFullProcessedPath;
  }
  else
  {
    *anOutStream = NULL;
  }
  return S_OK;
}

STDMETHODIMP CExtractCallBack200Imp::PrepareOperation(INT32 anAskExtractMode)
{
  m_ExtractMode = false;
  switch (anAskExtractMode)
  {
    case NArchiveHandler::NExtract::NAskMode::kExtract:
      m_ExtractMode = true;
  };
  return m_ExtractCallback2->PrepareOperation(m_FilePath, anAskExtractMode);

  return S_OK;
}

void CExtractCallBack200Imp::AddErrorMessage(LPCTSTR aMessage)
{
  m_Messages.Add(aMessage);
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
      m_OutFileStream.Release();
      return E_FAIL;
  }
  if(m_OutFileStream != NULL)
    m_OutFileStreamSpec->m_File.SetLastWriteTime(&m_ProcessedFileInfo.UTCLastWriteTime);
  m_OutFileStream.Release();
  if (m_ExtractMode)
    SetFileAttributes(m_DiskFilePath, m_ProcessedFileInfo.Attributes);
  RETURN_IF_NOT_S_OK(m_ExtractCallback2->OperationResult(anOperationResult));
  return S_OK;
}

STDMETHODIMP CExtractCallBack200Imp::CryptoGetTextPassword(BSTR *aPassword)
{
  if (!m_CryptoGetTextPassword)
  {
    RETURN_IF_NOT_S_OK(m_ExtractCallback2.QueryInterface(&m_CryptoGetTextPassword));
  }
  return m_CryptoGetTextPassword->CryptoGetTextPassword(aPassword);
}

