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

#include "../../Explorer/ProcessMessages.h"

using namespace NWindows;
using namespace NZipSettings;

CExtractCallBackImp::~CExtractCallBackImp()
{
  m_ProgressDialog.Destroy();
}

void CExtractCallBackImp::Init(IArchiveHandler200 *anArchiveHandler,
    const CSysString &aDirectoryPath,   
    const UString &anItemDefaultName,
    const FILETIME &anUTCLastWriteTimeDefault,
    UINT32 anAttributesDefault)
{
  m_CodePage = CP_ACP;
  m_NumErrors = 0;
  m_ItemDefaultName = anItemDefaultName;
  m_UTCLastWriteTimeDefault = anUTCLastWriteTimeDefault;
  m_AttributesDefault = anAttributesDefault;
  m_ArchiveHandler = anArchiveHandler;
  m_DirectoryPath = aDirectoryPath;
  NFile::NName::NormalizeDirPathPrefix(m_DirectoryPath);
}

STDMETHODIMP CExtractCallBackImp::SetTotal(UINT64 aSize)
{
  if (m_ThreadID != GetCurrentThreadId())
    return S_OK;
  m_ProgressDialog.SetRange(aSize);
  m_ProgressDialog.SetPos(0);
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::SetCompleted(const UINT64 *aCompleteValue)
{
  if (m_ThreadID != GetCurrentThreadId())
    return S_OK;
  ProcessMessages(m_ProgressDialog);
  if(m_ProgressDialog.WasProcessStopped())
    return E_ABORT;
  if (aCompleteValue != NULL)
    m_ProgressDialog.SetPos(*aCompleteValue);
  return S_OK;
}

void CExtractCallBackImp::CreateComplexDirectory(const UStringVector &aDirPathParts)
{
  CSysString aFullPath = m_DirectoryPath;
  for(int i = 0; i < aDirPathParts.Size(); i++)
  {
    aFullPath += GetSystemString(aDirPathParts[i], m_CodePage);
    NFile::NDirectory::MyCreateDirectory(aFullPath);
    aFullPath += NFile::NName::kDirDelimiter;
  }
}

STDMETHODIMP CExtractCallBackImp::Extract(UINT32 anIndex,
    ISequentialOutStream **anOutStream, INT32 anAskExtractMode)
{
  ProcessMessages(m_ProgressDialog);
  if(m_ProgressDialog.WasProcessStopped())
    return E_ABORT;
  m_OutFileStream.Release();
  NCOM::CPropVariant aPropVariantName;
  RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidPath, &aPropVariantName));
  UString aFullPath;
  if(aPropVariantName.vt == VT_EMPTY)
    aFullPath = m_ItemDefaultName;
  else 
  {
    if(aPropVariantName.vt != VT_BSTR)
      return E_FAIL;
    aFullPath = aPropVariantName.bstrVal;
  }
  m_FilePath = GetSystemString(aFullPath, m_CodePage);

  // m_CurrentFilePath = GetSystemString(aFullPath, m_CodePage);
  
  if(anAskExtractMode == NArchiveHandler::NExtract::NAskMode::kExtract)
  {
    NCOM::CPropVariant aPropVariant;
    RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidAttributes, &aPropVariant));
    if (aPropVariant.vt == VT_EMPTY)
      m_ProcessedFileInfo.Attributes = m_AttributesDefault;
    else
    {
      if (aPropVariant.vt != VT_UI4)
        return E_FAIL;
      m_ProcessedFileInfo.Attributes = aPropVariant.ulVal;
    }

    RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidIsFolder, &aPropVariant));
    m_ProcessedFileInfo.IsDirectory = VARIANT_BOOLToBool(aPropVariant.boolVal);

    bool anIsAnti = false;
    {
      NCOM::CPropVariant aPropVariantTemp;
      RETURN_IF_NOT_S_OK(m_ArchiveHandler->GetProperty(anIndex, kaipidIsAnti, 
          &aPropVariantTemp));
      if (aPropVariantTemp.vt != VT_EMPTY)
        anIsAnti = VARIANT_BOOLToBool(aPropVariantTemp.boolVal);
    }

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

    UStringVector aPathParts; 
    SplitPathToParts(aFullPath, aPathParts);
    if(aPathParts.IsEmpty())
      return E_FAIL;

    UString aProcessedPath;
    aProcessedPath = aFullPath;

    if(!m_ProcessedFileInfo.IsDirectory)
      aPathParts.DeleteBack();
    if (!aPathParts.IsEmpty())
    {
      if (!anIsAnti)
        CreateComplexDirectory(aPathParts);
    }

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
      if (!NFile::NDirectory::DeleteFileAlways(aFullProcessedPath))
      {
        MessageBox(0, "Can not delete output file", "7-Zip", 0);
        // g_StdOut << GetOemString(aFullProcessedPath);
        // return E_ABORT;
        return E_ABORT;
      }
    }

    if (!anIsAnti)
    {
      m_OutFileStreamSpec = new CComObjectNoLock<COutFileStream>;
      CComPtr<ISequentialOutStream> anOutStreamLoc(m_OutFileStreamSpec);
      if (!m_OutFileStreamSpec->Open(aFullProcessedPath))
      {
        MessageBox(0, "Can not open output file", "7-Zip", 0);
        // g_StdOut << GetOemString(aFullProcessedPath);
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

STDMETHODIMP CExtractCallBackImp::PrepareOperation(INT32 anAskExtractMode)
{
  m_ExtractMode = false;
  switch (anAskExtractMode)
  {
    case NArchiveHandler::NExtract::NAskMode::kExtract:
      m_ExtractMode = true;
      break;
  };
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::OperationResult(INT32 aResultEOperationResult)
{
  switch(aResultEOperationResult)
  {
    case NArchiveHandler::NExtract::NOperationResult::kOK:
    {
      break;
    }
    default:
    {
      m_NumErrors++;
      switch(aResultEOperationResult)
      {
        case NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod:
          MessageBox(0, "Unsupported Method", "7-Zip", 0);
          return E_FAIL;
          // break;
        case NArchiveHandler::NExtract::NOperationResult::kCRCError:
          MessageBox(0, "CRC Failed", "7-Zip", 0);
          return E_FAIL;
          // break;
        case NArchiveHandler::NExtract::NOperationResult::kDataError:
          MessageBox(0, "Data Error", "7-Zip", 0);
          return E_FAIL;
          // break;
        default:
          m_OutFileStream.Release();
          return E_FAIL;
      }
    }
  }
  if(m_OutFileStream != NULL)
    m_OutFileStreamSpec->m_File.SetLastWriteTime(&m_ProcessedFileInfo.UTCLastWriteTime);
  m_OutFileStream.Release();
  if (m_ExtractMode)
    SetFileAttributes(m_DiskFilePath, m_ProcessedFileInfo.Attributes);
  return S_OK;
}

 
