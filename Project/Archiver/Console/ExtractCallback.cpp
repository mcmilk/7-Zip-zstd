// ExtractCallback.h

#include "StdAfx.h"

#include "ExtractCallback.h"
#include "UserInputUtils.h"

#include "ConsoleCloseUtils.h"

#include "Common/StdOutStream.h"
#include "Common/StdInStream.h"
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
using namespace NZipSettings;

static const char *kTestingString    =  "Testing     ";
static const char *kExtractingString =  "Extracting  ";
static const char *kSkippingString   =  "Skipping    ";

void CExtractCallBackImp::Init(IArchiveHandler200 *anArchiveHandler,
    const CSysString &aDirectoryPath, const NZipSettings::NExtraction::CInfo 
        &aExtractModeInfo,
    const UStringVector &aRemovePathParts,
    UINT aCodePage, 
    const UString &anItemDefaultName,
    const FILETIME &anUTCLastWriteTimeDefault,
    UINT32 anAttributesDefault,
    bool aPasswordIsDefined, 
    const UString &aPassword)
{
  m_PasswordIsDefined = aPasswordIsDefined;
  m_Password = aPassword;
  m_NumErrors = 0;

  m_ItemDefaultName = anItemDefaultName;
  m_UTCLastWriteTimeDefault = anUTCLastWriteTimeDefault;
  m_AttributesDefault = anAttributesDefault;
  
  m_CodePage = aCodePage;
  m_RemovePathParts = aRemovePathParts;
  m_ExtractModeInfo = aExtractModeInfo;
  m_ArchiveHandler = anArchiveHandler;
  m_DirectoryPath = aDirectoryPath;
  NFile::NName::NormalizeDirPathPrefix(m_DirectoryPath);
}

bool CExtractCallBackImp::IsEncrypted(UINT32 anIndex)
{
  NCOM::CPropVariant aPropVariant;
  if(m_ArchiveHandler->GetProperty(anIndex, kaipidEncrypted, &aPropVariant) != S_OK)
    return false;
  if (aPropVariant.vt != VT_BOOL)
    return false;
  return VARIANT_BOOLToBool(aPropVariant.boolVal);
}
  
STDMETHODIMP CExtractCallBackImp::SetTotal(UINT64 aSize)
{
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::SetCompleted(const UINT64 *aCompleteValue)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
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

STDMETHODIMP CExtractCallBackImp::Extract(UINT32 anIndex,
    ISequentialOutStream **anOutStream, INT32 anAskExtractMode)
{
  if (NConsoleClose::TestBreakSignal())
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
        throw "incorrect item";
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

    // GetPropertyValue(anItemIDList, kaipidSize, &aPropVariant);
    // UINT64 aNewFileSize = ConvertPropVariantToUINT64(aPropVariant);

    UStringVector aPathParts; 
    SplitPathToParts(aFullPath, aPathParts);
    if(aPathParts.IsEmpty())
      return E_FAIL;
    UString aProcessedPath;
    switch(m_ExtractModeInfo.PathMode)
    {
      case NExtraction::NPathMode::kFullPathnames:
      {
        aProcessedPath = aFullPath;
        break;
      }
      case NExtraction::NPathMode::kCurrentPathnames:
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
      case NExtraction::NPathMode::kNoPathnames:
      {
        aProcessedPath = aPathParts.Back(); 
        aPathParts.Delete(0, aPathParts.Size() - 1); // Test it!!
        break;
      }
    }
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
      switch(m_ExtractModeInfo.OverwriteMode)
      {
        case NExtraction::NOverwriteMode::kSkipExisting:
          return S_OK;
        case NExtraction::NOverwriteMode::kAskBefore:
        {
          /*
          NOverwriteDialog::CFileInfo anOldFileInfo, aNewFileInfo;
          anOldFileInfo.Time = aFileInfo.LastWriteTime;
          anOldFileInfo.Size = aFileInfo.Size;
          anOldFileInfo.Name = aFullProcessedPath;
          
          aNewFileInfo.Time = m_ProcessedFileInfo.UTCLastWriteTime;
          aNewFileInfo.Size = aNewFileSize;
          aNewFileInfo.Name = GetSystemString(aFullPath, m_CodePage);

          NOverwriteDialog::NResult::EEnum aResult = 
              NOverwriteDialog::Execute(anOldFileInfo, aNewFileInfo);
          */

          g_StdOut << "file " << GetOemString(aFullProcessedPath) << 
              "\nalready exists. Overwrite with " << endl;
          g_StdOut << UnicodeStringToMultiByte(aFullPath, CP_OEMCP);

          NUserAnswerMode::EEnum anOverwriteAnswer = ScanUserYesNoAllQuit();

          switch(anOverwriteAnswer)
          {
          case NUserAnswerMode::kQuit:
            return E_ABORT;
          case NUserAnswerMode::kNo:
            return S_OK;
          case NUserAnswerMode::kNoAll:
            m_ExtractModeInfo.OverwriteMode = NExtraction::NOverwriteMode::kSkipExisting;
            return S_OK;
          case NUserAnswerMode::kYesAll:
            m_ExtractModeInfo.OverwriteMode = NExtraction::NOverwriteMode::kWithoutPrompt;
            break;
          case NUserAnswerMode::kYes:
            break;
          case NUserAnswerMode::kAutoRename:
            m_ExtractModeInfo.OverwriteMode = NExtraction::NOverwriteMode::kAutoRename;
            break;
          default:
            throw 20413;
          }
          break;
        }
      }
      if (m_ExtractModeInfo.OverwriteMode == NExtraction::NOverwriteMode::kAutoRename)
      {
        if (!AutoRenamePath(aFullProcessedPath))
        {
          g_StdOut << "can not create file with auto name " << endl;
          g_StdOut << GetOemString(aFullProcessedPath);
          return E_ABORT;
        }
      }
      else
        if (!NFile::NDirectory::DeleteFileAlways(aFullProcessedPath))
        {
          g_StdOut << "can not delete output file " << endl;
          g_StdOut << GetOemString(aFullProcessedPath);
          return E_ABORT;
        }
    }

    if (!anIsAnti)
    {
      m_OutFileStreamSpec = new CComObjectNoLock<COutFileStream>;
      CComPtr<ISequentialOutStream> anOutStreamLoc(m_OutFileStreamSpec);
      if (!m_OutFileStreamSpec->Open(aFullProcessedPath))
      {
        g_StdOut << "can not open output file " << endl;
        g_StdOut << GetOemString(aFullProcessedPath);
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
      g_StdOut << kExtractingString;
      break;
    case NArchiveHandler::NExtract::NAskMode::kTest:
      g_StdOut << kTestingString;
      break;
    case NArchiveHandler::NExtract::NAskMode::kSkip:
      g_StdOut << kSkippingString;
      break;
  };
  g_StdOut << GetOemString(m_FilePath);
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
          g_StdOut << "     Unsupported Method";
          break;
        case NArchiveHandler::NExtract::NOperationResult::kCRCError:
          g_StdOut << "     CRC Failed";
          break;
        case NArchiveHandler::NExtract::NOperationResult::kDataError:
          g_StdOut << "     Data Error";
          break;
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
  g_StdOut << endl;
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::CryptoGetTextPassword(BSTR *aPassword)
{
  if (!m_PasswordIsDefined)
  {
    g_StdOut << "\nEnter password:";
    AString anOemPassword = g_StdIn.ScanStringUntilNewLine();
    m_Password = MultiByteToUnicodeString(anOemPassword, CP_OEMCP); 
    m_PasswordIsDefined = true;
  }
  CComBSTR aTempName = m_Password;
  *aPassword = aTempName.Detach();
  return S_OK;
}
  
