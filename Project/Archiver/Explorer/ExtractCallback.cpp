// ExtractCallback.h

#include "StdAfx.h"

#include "ExtractCallback.h"

#include "Windows/FileFind.h"
#include "Windows/FileDir.h"

#include "../Resource/OverwriteDialog/OverwriteDialog.h"
#include "../Resource/PasswordDialog/PasswordDialog.h"
#include "../Resource/MessagesDialog/MessagesDialog.h"
#include "../Resource/Extract/resource.h"

#include "Common/WildCard.h"
#include "Common/StringConvert.h"

#include "ProcessMessages.h"
#include "FormatUtils.h"

#include "../Common/ExtractAutoRename.h"


using namespace NWindows;
using namespace NFile;
using namespace NFind;

void CExtractCallBackImp::DestroyWindows()
{
  m_ProgressDialog.Destroy();
}

CExtractCallBackImp::~CExtractCallBackImp()
{
  m_ProgressDialog.Destroy();

  // m_ProgressDialog.Release();
  if (!m_Messages.IsEmpty())
  {
    CMessagesDialog aMessagesDialog;
    aMessagesDialog.m_Messages = &m_Messages;
    aMessagesDialog.Create(m_ParentWindow);
  }
}

static inline UINT GetCurrentFileCodePage()
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

void CExtractCallBackImp::Init(
    NExtractionMode::NOverwrite::EEnum anOverwriteMode,
    bool aPasswordIsDefined, 
    const UString &aPassword)
{
  m_OverwriteMode = anOverwriteMode;
  m_PasswordIsDefined = aPasswordIsDefined;
  m_Password = aPassword;
  m_Messages.Clear();
  m_FileCodePage = GetCurrentFileCodePage();
}

void CExtractCallBackImp::AddErrorMessage(LPCTSTR aMessage)
{
  m_Messages.Add(aMessage);
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
  // if(m_ProgressDialog.HasUserCancelled())
    return E_ABORT;
  if (aCompleteValue != NULL)
    m_ProgressDialog.SetPos(*aCompleteValue);
  // m_ProgressDialog.SetProgress(*aCompleteValue, m_Total);
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::AskOverwrite(
    const wchar_t *anExistName, const FILETIME *anExistTime, const UINT64 *anExistSize,
    const wchar_t *aNewName, const FILETIME *aNewTime, const UINT64 *aNewSize,
    INT32 *anAnswer)
{
  COverwriteDialog aDialog;

  // NOverwriteDialog::CFileInfo anOldFileInfo, aNewFileInfo;
  aDialog.m_OldFileInfo.Time = *anExistTime;
  if (aDialog.m_OldFileInfo.SizeIsDefined = (anExistSize != NULL))
    aDialog.m_OldFileInfo.Size = *anExistSize;
  aDialog.m_OldFileInfo.Name = GetSystemString(anExistName);

 
  aDialog.m_NewFileInfo.Time = *aNewTime;
  
  if (aDialog.m_NewFileInfo.SizeIsDefined = (aNewSize != NULL))
    aDialog.m_NewFileInfo.Size = *aNewSize;
  aDialog.m_NewFileInfo.Name = GetSystemString(aNewName);
  
  /*
  NOverwriteDialog::NResult::EEnum aResult = 
    NOverwriteDialog::Execute(anOldFileInfo, aNewFileInfo);
  */
  int aResult = aDialog.Create(NULL); // m_ParentWindow doesn't work with 7z
  
  switch(aResult)
  {
  case IDCANCEL:
    return E_ABORT;
    // anAskResult = NAskOverwriteAnswer::kCancel;
    // break;
  case IDNO:
    *anAnswer = NOverwriteAnswer::kNo;
    break;
  case IDC_BUTTON_OVERWRITE_NO_TO_ALL:
    *anAnswer = NOverwriteAnswer::kNoToAll;
    break;
  case IDC_BUTTON_OVERWRITE_YES_TO_ALL:
    *anAnswer = NOverwriteAnswer::kYesToAll;
    break;
  case IDC_BUTTON_OVERWRITE_AUTO_RENAME:
    *anAnswer = NOverwriteAnswer::kAutoRename;
    break;
  case IDYES:
    *anAnswer = NOverwriteAnswer::kYes;
    break;
  default:
    throw 20413;
  }
  return S_OK;
}


STDMETHODIMP CExtractCallBackImp::PrepareOperation(const wchar_t *aName, INT32 anAskExtractMode)
{
  m_CurrentFilePath = aName;
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::MessageError(const wchar_t *aMessage)
{
  CSysString aString = GetSystemString(aMessage);
  /*
  if (g_StartupInfo.ShowMessage(aString) == -1)
    return E_ABORT;
    */
  AddErrorMessage(aString);

  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::OperationResult(INT32 anOperationResult)
{
  switch(anOperationResult)
  {
    case NArchiveHandler::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT anIDMessage;
      UINT32 aLangID;
      switch(anOperationResult)
      {
        case NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod:
          anIDMessage = IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_UNSUPPORTED_METHOD;
          aLangID = 0x02000A91;
          break;
        case NArchiveHandler::NExtract::NOperationResult::kDataError:
          anIDMessage = IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_DATA_ERROR;
          aLangID = 0x02000A92;
          break;
        case NArchiveHandler::NExtract::NOperationResult::kCRCError:
          anIDMessage = IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_CRC;
          aLangID = 0x02000A93;
          break;
        default:
          return E_FAIL;
      }
      AddErrorMessage(MyFormat(anIDMessage, 
          #ifdef LANG 
          aLangID, 
          #endif 
          GetSystemString(m_CurrentFilePath)));
    }
  }
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::CryptoGetTextPassword(BSTR *aPassword)
{
  if (!m_PasswordIsDefined)
  {
    CPasswordDialog aDialog;
   
    if (aDialog.Create(m_ParentWindow) == IDCANCEL)
      return E_ABORT;

    m_Password = GetUnicodeString((LPCTSTR)aDialog.m_Password);
    m_PasswordIsDefined = true;
  }
  CComBSTR aTempName = m_Password;
  *aPassword = aTempName.Detach();

  return S_OK;
}


// IExtractCallBack3
STDMETHODIMP CExtractCallBackImp::AskWrite(
    const wchar_t *aSrcPath, INT32 _aSrcIsFolder, 
    const FILETIME *aSrcTime, const UINT64 *aSrcSize,
    const wchar_t *_aDestPath, 
    BSTR *_aDestPathResult, 
    INT32 *aResult)
{
  CComBSTR aDestPathResult = _aDestPath;
  *_aDestPathResult = aDestPathResult.Detach();

  UString aDestPath = _aDestPath;
  CSysString aDestPathSys = GetSystemString(aDestPath, m_FileCodePage);
  *aResult = BoolToMyBool(false);
  bool aSrcIsFolder = MyBoolToBool(_aSrcIsFolder);
  CFileInfo aDestFileInfo;
  if (FindFile(aDestPathSys, aDestFileInfo))
  {
    if (aSrcIsFolder)
    {
      if (!aDestFileInfo.IsDirectory())
      {
        UString aMessage = UString(L"can not replace file \'")
          + aDestPath +
          UString(L"\' with folder with same name");
        RETURN_IF_NOT_S_OK(MessageError(aMessage));
        return E_ABORT;
      }
      *aResult = BoolToMyBool(false);
      return S_OK;
    }
    if (aDestFileInfo.IsDirectory())
    {
      UString aMessage = UString(L"can not replace folder \'")
          + aDestPath +
          UString(L"\' with file with same name");
      RETURN_IF_NOT_S_OK(MessageError(aMessage));
      return E_FAIL;
    }

    switch(m_OverwriteMode)
    {
      case NExtractionMode::NOverwrite::kSkipExisting:
        return S_OK;
      case NExtractionMode::NOverwrite::kAskBefore:
      {
        INT32 aOverwiteResult;
        RETURN_IF_NOT_S_OK(AskOverwrite(
            aDestPath, 
            &aDestFileInfo.LastWriteTime, &aDestFileInfo.Size,
            GetUnicodeString(aSrcPath, m_FileCodePage),
            aSrcTime, aSrcSize, 
            &aOverwiteResult));
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
      if (!AutoRenamePath(aDestPathSys))
      {
        UString aMessage = UString(L"can not create name of file ")
            + GetUnicodeString(aDestPathSys, m_FileCodePage);
        RETURN_IF_NOT_S_OK(MessageError(aMessage));
        return E_ABORT;
      }
      {
        CComBSTR aDestPathResultPrev;
        aDestPathResult.Attach(*_aDestPathResult);
      }
      CComBSTR aDestPathResult = GetUnicodeString(aDestPathSys, m_FileCodePage);
      *_aDestPathResult = aDestPathResult.Detach();
    }
    else
      if (!NFile::NDirectory::DeleteFileAlways(aDestPathSys))
      {
        UString aMessage = UString(L"can not delete output file ")
            + GetUnicodeString(aDestPathSys, m_FileCodePage);
        RETURN_IF_NOT_S_OK(MessageError(aMessage));
        return E_ABORT;
      }
  }
  *aResult = BoolToMyBool(true);
  return S_OK;
}

