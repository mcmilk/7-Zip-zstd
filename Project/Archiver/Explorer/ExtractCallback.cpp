// ExtractCallback.h

#include "StdAfx.h"

#include "ExtractCallback.h"

#include "OverwriteDialog.h"
#include "PasswordDialog.h"

#include "Common/WildCard.h"
#include "Common/StringConvert.h"

#include "ProcessMessages.h"
#include "FormatUtils.h"

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

void CExtractCallBackImp::Init(IArchiveHandler100 *anArchiveHandler,
    NExtractionDialog::CModeInfo anExtractModeInfo,
    bool aPasswordIsDefined, 
    const UString &aPassword)
{
  m_ArchiveHandler = anArchiveHandler;
  m_PasswordIsDefined = aPasswordIsDefined;
  m_Password = aPassword;
  m_ExtractModeInfo = anExtractModeInfo;
  m_Messages.Clear();
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

