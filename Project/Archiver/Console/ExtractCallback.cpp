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

#include "Util/FilePathAutoRename.h"

using namespace NWindows;
using namespace NZipSettings;

static const char *kTestingString    =  "Testing     ";
static const char *kExtractingString =  "Extracting  ";
static const char *kSkippingString   =  "Skipping    ";

void CExtractCallbackImp::Init(IInArchive *archive,
    const CSysString &directoryPath, 
    const NZipSettings::NExtraction::CInfo &extractModeInfo,
    const UStringVector &removePathParts,
    UINT codePage, 
    const UString &itemDefaultName,
    const FILETIME &utcLastWriteTimeDefault,
    UINT32 attributesDefault,
    bool passwordIsDefined, 
    const UString &password)
{
  m_PasswordIsDefined = passwordIsDefined;
  m_Password = password;
  m_NumErrors = 0;

  m_ItemDefaultName = itemDefaultName;
  m_UTCLastWriteTimeDefault = utcLastWriteTimeDefault;
  m_AttributesDefault = attributesDefault;
  
  m_CodePage = codePage;
  m_RemovePathParts = removePathParts;
  m_ExtractModeInfo = extractModeInfo;
  m_ArchiveHandler = archive;
  m_DirectoryPath = directoryPath;
  NFile::NName::NormalizeDirPathPrefix(m_DirectoryPath);
}

bool CExtractCallbackImp::IsEncrypted(UINT32 index)
{
  NCOM::CPropVariant propVariant;
  if(m_ArchiveHandler->GetProperty(index, kpidEncrypted, &propVariant) != S_OK)
    return false;
  if (propVariant.vt != VT_BOOL)
    return false;
  return VARIANT_BOOLToBool(propVariant.boolVal);
}
  
STDMETHODIMP CExtractCallbackImp::SetTotal(UINT64 size)
{
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompleted(const UINT64 *completeValue)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

void CExtractCallbackImp::CreateComplexDirectory(const UStringVector &dirPathParts)
{
  CSysString fullPath = m_DirectoryPath;
  for(int i = 0; i < dirPathParts.Size(); i++)
  {
    fullPath += GetSystemString(dirPathParts[i], m_CodePage);
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

STDMETHODIMP CExtractCallbackImp::GetStream(UINT32 index,
    ISequentialOutStream **outStream, INT32 askExtractMode)
{
  *outStream = NULL;
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  m_OutFileStream.Release();
  NCOM::CPropVariant propVariantName;
  RINOK(m_ArchiveHandler->GetProperty(index, kpidPath, &propVariantName));
  UString fullPath;
  if(propVariantName.vt == VT_EMPTY)
    fullPath = m_ItemDefaultName;
  else 
  {
    if(propVariantName.vt != VT_BSTR)
      return E_FAIL;
    fullPath = propVariantName.bstrVal;
  }
  m_FilePath = GetSystemString(fullPath, m_CodePage);

  // m_CurrentFilePath = GetSystemString(fullPath, m_CodePage);
  
  if(askExtractMode == NArchive::NExtract::NAskMode::kExtract)
  {
    NCOM::CPropVariant propVariant;
    RINOK(m_ArchiveHandler->GetProperty(index, kpidAttributes, &propVariant));
    if (propVariant.vt == VT_EMPTY)
    {
      m_ProcessedFileInfo.Attributes = m_AttributesDefault;
      m_ProcessedFileInfo.AttributesAreDefined = false;
    }
    else
    {
      if (propVariant.vt != VT_UI4)
        throw "incorrect item";
      m_ProcessedFileInfo.Attributes = propVariant.ulVal;
      m_ProcessedFileInfo.AttributesAreDefined = true;
    }

    RINOK(m_ArchiveHandler->GetProperty(index, kpidIsFolder, &propVariant));
    m_ProcessedFileInfo.IsDirectory = VARIANT_BOOLToBool(propVariant.boolVal);

    bool isAnti = false;
    {
      NCOM::CPropVariant propVariantTemp;
      RINOK(m_ArchiveHandler->GetProperty(index, kpidIsAnti, 
          &propVariantTemp));
      if (propVariantTemp.vt == VT_BOOL)
        isAnti = VARIANT_BOOLToBool(propVariantTemp.boolVal);
    }

    RINOK(m_ArchiveHandler->GetProperty(index, kpidLastWriteTime, &propVariant));
    switch(propVariant.vt)
    {
      case VT_EMPTY:
        m_ProcessedFileInfo.UTCLastWriteTime = m_UTCLastWriteTimeDefault;
        break;
      case VT_FILETIME:
        m_ProcessedFileInfo.UTCLastWriteTime = propVariant.filetime;
        break;
      default:
        return E_FAIL;
    }

    // GetPropertyValue(anItemIDList, kpidSize, &propVariant);
    // UINT64 newFileSize = ConvertPropVariantToUINT64(propVariant);

    UStringVector pathParts; 
    SplitPathToParts(fullPath, pathParts);
    if(pathParts.IsEmpty())
      return E_FAIL;
    UString processedPath;
    switch(m_ExtractModeInfo.PathMode)
    {
      case NExtraction::NPathMode::kFullPathnames:
      {
        processedPath = fullPath;
        break;
      }
      case NExtraction::NPathMode::kCurrentPathnames:
      {
        int numRemovePathParts = m_RemovePathParts.Size();
        if(pathParts.Size() <= numRemovePathParts)
          return E_FAIL;
        for(int i = 0; i < numRemovePathParts; i++)
          if(m_RemovePathParts[i].CollateNoCase(pathParts[i]) != 0)
            return E_FAIL;
        pathParts.Delete(0, numRemovePathParts);
        processedPath = MakePathNameFromParts(pathParts);
        break;
      }
      case NExtraction::NPathMode::kNoPathnames:
      {
        processedPath = pathParts.Back(); 
        pathParts.Delete(0, pathParts.Size() - 1); // Test it!!
        break;
      }
    }
    if(!m_ProcessedFileInfo.IsDirectory)
      pathParts.DeleteBack();
    if (!pathParts.IsEmpty())
    {
      if (!isAnti)
        CreateComplexDirectory(pathParts);
    }

    CSysString fullProcessedPath = m_DirectoryPath + 
        GetSystemString(processedPath, m_CodePage);

    if(m_ProcessedFileInfo.IsDirectory)
    {
      m_DiskFilePath = fullProcessedPath;

      if (isAnti)
        ::RemoveDirectory(m_DiskFilePath);
      return S_OK;
    }

    NFile::NFind::CFileInfo fileInfo;
    if(NFile::NFind::FindFile(fullProcessedPath, fileInfo))
    {
      switch(m_ExtractModeInfo.OverwriteMode)
      {
        case NExtraction::NOverwriteMode::kSkipExisting:
          return S_OK;
        case NExtraction::NOverwriteMode::kAskBefore:
        {
          /*
          NOverwriteDialog::CFileInfo oldFileInfo, newFileInfo;
          oldFileInfo.Time = fileInfo.LastWriteTime;
          oldFileInfo.Size = fileInfo.Size;
          oldFileInfo.Name = fullProcessedPath;
          
          newFileInfo.Time = m_ProcessedFileInfo.UTCLastWriteTime;
          newFileInfo.Size = newFileSize;
          newFileInfo.Name = GetSystemString(fullPath, m_CodePage);

          NOverwriteDialog::NResult::EEnum result = 
              NOverwriteDialog::Execute(oldFileInfo, newFileInfo);
          */

          g_StdOut << "file " << GetOemString(fullProcessedPath) << 
              "\nalready exists. Overwrite with " << endl;
          g_StdOut << UnicodeStringToMultiByte(fullPath, CP_OEMCP);

          NUserAnswerMode::EEnum overwriteAnswer = ScanUserYesNoAllQuit();

          switch(overwriteAnswer)
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
        if (!AutoRenamePath(fullProcessedPath))
        {
          g_StdOut << "can not create file with auto name " << endl;
          g_StdOut << GetOemString(fullProcessedPath);
          return E_ABORT;
        }
      }
      else
        if (!NFile::NDirectory::DeleteFileAlways(fullProcessedPath))
        {
          g_StdOut << "can not delete output file " << endl;
          g_StdOut << GetOemString(fullProcessedPath);
          return E_ABORT;
        }
    }

    if (!isAnti)
    {
      m_OutFileStreamSpec = new CComObjectNoLock<COutFileStream>;
      CComPtr<ISequentialOutStream> outStreamLoc(m_OutFileStreamSpec);
      if (!m_OutFileStreamSpec->Open(fullProcessedPath))
      {
        m_NumErrors++;
        g_StdOut << "Can not open output file " << endl;
        g_StdOut << GetOemString(fullProcessedPath) << endl;
        return S_OK;
      }
      m_OutFileStream = outStreamLoc;
      *outStream = outStreamLoc.Detach();
    }
    m_DiskFilePath = fullProcessedPath;
  }
  else
  {
    *outStream = NULL;
  }
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::PrepareOperation(INT32 askExtractMode)
{
  m_ExtractMode = false;
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      m_ExtractMode = true;
      g_StdOut << kExtractingString;
      break;
    case NArchive::NExtract::NAskMode::kTest:
      g_StdOut << kTestingString;
      break;
    case NArchive::NExtract::NAskMode::kSkip:
      g_StdOut << kSkippingString;
      break;
  };
  g_StdOut << GetOemString(m_FilePath);
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
      m_NumErrors++;
      switch(resultEOperationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          g_StdOut << "     Unsupported Method";
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          g_StdOut << "     CRC Failed";
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          g_StdOut << "     Data Error";
          break;
        default:
          g_StdOut << "     Unknown Error";
          // m_OutFileStream.Release();
          // return E_FAIL;
      }
    }
  }
  if(m_OutFileStream != NULL)
    m_OutFileStreamSpec->File.SetLastWriteTime(&m_ProcessedFileInfo.UTCLastWriteTime);
  m_OutFileStream.Release();
  if (m_ExtractMode && m_ProcessedFileInfo.AttributesAreDefined)
    SetFileAttributes(m_DiskFilePath, m_ProcessedFileInfo.Attributes);
  g_StdOut << endl;
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::CryptoGetTextPassword(BSTR *password)
{
  if (!m_PasswordIsDefined)
  {
    g_StdOut << "\nEnter password:";
    AString oemPassword = g_StdIn.ScanStringUntilNewLine();
    m_Password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
    m_PasswordIsDefined = true;
  }
  CComBSTR tempName = m_Password;
  *password = tempName.Detach();
  return S_OK;
}
  
