// ExtractCallback.h

#include "StdAfx.h"

#include "ExtractCallback.h"

#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/ProcessMessages.h"

#include "Resource/OverwriteDialog/OverwriteDialog.h"
#include "Resource/PasswordDialog/PasswordDialog.h"
#include "Resource/MessagesDialog/MessagesDialog.h"
#include "../Archiver/Resource/Extract/resource.h"

#include "Common/WildCard.h"
#include "Common/StringConvert.h"

#include "FormatUtils.h"

#include "Util/FilePathAutoRename.h"


using namespace NWindows;
using namespace NFile;
using namespace NFind;

void CExtractCallbackImp::DestroyWindows()
{
  _progressDialog.Destroy();
}

CExtractCallbackImp::~CExtractCallbackImp()
{
  _progressDialog.Destroy();

  // _progressDialog.Release();
  if (!_messages.IsEmpty())
  {
    CMessagesDialog messagesDialog;
    messagesDialog._messages = &_messages;
    messagesDialog.Create(_parentWindow);
  }
}

static inline UINT GetCurrentFileCodePage()
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

void CExtractCallbackImp::Init(
    NExtractionMode::NOverwrite::EEnum overwriteMode,
    bool passwordIsDefined, 
    const UString &password)
{
  _overwriteMode = overwriteMode;
  _passwordIsDefined = passwordIsDefined;
  _password = password;
  _messages.Clear();
  _fileCodePage = GetCurrentFileCodePage();
}

void CExtractCallbackImp::AddErrorMessage(LPCTSTR message)
{
  _messages.Add(message);
}

STDMETHODIMP CExtractCallbackImp::SetTotal(UINT64 aSize)
{
  if (_threadID != GetCurrentThreadId())
    return S_OK;
  _progressDialog.SetRange(aSize);
  _progressDialog.SetPos(0);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompleted(const UINT64 *aCompleteValue)
{
  if (_threadID != GetCurrentThreadId())
    return S_OK;
  ProcessMessages(_progressDialog);
  if(_progressDialog.WasProcessStopped())
  // if(_progressDialog.HasUserCancelled())
    return E_ABORT;
  if (aCompleteValue != NULL)
    _progressDialog.SetPos(*aCompleteValue);
  // _progressDialog.SetProgress(*aCompleteValue, m_Total);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UINT64 *existSize,
    const wchar_t *newName, const FILETIME *newTime, const UINT64 *newSize,
    INT32 *answer)
{
  COverwriteDialog dialog;

  // NOverwriteDialog::CFileInfo anOldFileInfo, aNewFileInfo;
  dialog._oldFileInfo.Time = *existTime;
  if (dialog._oldFileInfo.SizeIsDefined = (existSize != NULL))
    dialog._oldFileInfo.Size = *existSize;
  dialog._oldFileInfo.Name = GetSystemString(existName);


  if (newTime == 0)
    dialog._newFileInfo.TimeIsDefined = false;
  else
  {
    dialog._newFileInfo.TimeIsDefined = true;
    dialog._newFileInfo.Time = *newTime;
  }
  
  if (dialog._newFileInfo.SizeIsDefined = (newSize != NULL))
    dialog._newFileInfo.Size = *newSize;
  dialog._newFileInfo.Name = GetSystemString(newName);
  
  /*
  NOverwriteDialog::NResult::EEnum writeAnswer = 
    NOverwriteDialog::Execute(anOldFileInfo, aNewFileInfo);
  */
  int writeAnswer = dialog.Create(NULL); // _parentWindow doesn't work with 7z
  
  switch(writeAnswer)
  {
  case IDCANCEL:
    return E_ABORT;
    // anAskResult = NAskOverwriteAnswer::kCancel;
    // break;
  case IDNO:
    *answer = NOverwriteAnswer::kNo;
    break;
  case IDC_BUTTON_OVERWRITE_NO_TO_ALL:
    *answer = NOverwriteAnswer::kNoToAll;
    break;
  case IDC_BUTTON_OVERWRITE_YES_TO_ALL:
    *answer = NOverwriteAnswer::kYesToAll;
    break;
  case IDC_BUTTON_OVERWRITE_AUTO_RENAME:
    *answer = NOverwriteAnswer::kAutoRename;
    break;
  case IDYES:
    *answer = NOverwriteAnswer::kYes;
    break;
  default:
    throw 20413;
  }
  return S_OK;
}


STDMETHODIMP CExtractCallbackImp::PrepareOperation(const wchar_t *aName, INT32 anAskExtractMode)
{
  _currentFilePath = aName;
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::MessageError(const wchar_t *message)
{
  AddErrorMessage(GetSystemString(message));
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::ShowMessage(const wchar_t *message)
{
  AddErrorMessage(GetSystemString(message));
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::OperationResult(INT32 operationResult)
{
  switch(operationResult)
  {
    case NArchiveHandler::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT messageID;
      UINT32 langID;
      switch(operationResult)
      {
        case NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod:
          messageID = IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_UNSUPPORTED_METHOD;
          langID = 0x02000A91;
          break;
        case NArchiveHandler::NExtract::NOperationResult::kDataError:
          messageID = IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_DATA_ERROR;
          langID = 0x02000A92;
          break;
        case NArchiveHandler::NExtract::NOperationResult::kCRCError:
          messageID = IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_CRC;
          langID = 0x02000A93;
          break;
        default:
          return E_FAIL;
      }
      AddErrorMessage(
        GetSystemString(MyFormatNew(messageID, 
          #ifdef LANG 
          langID, 
          #endif 
          _currentFilePath)));
    }
  }
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::CryptoGetTextPassword(BSTR *password)
{
  if (!_passwordIsDefined)
  {
    CPasswordDialog dialog;
   
    if (dialog.Create(_parentWindow) == IDCANCEL)
      return E_ABORT;

    _password = GetUnicodeString((LPCTSTR)dialog._password);
    _passwordIsDefined = true;
  }
  CComBSTR tempName = _password;
  *password = tempName.Detach();

  return S_OK;
}


// IExtractCallBack3
STDMETHODIMP CExtractCallbackImp::AskWrite(
    const wchar_t *srcPath, INT32 srcIsFolder, 
    const FILETIME *srcTime, const UINT64 *aSrcSize,
    const wchar_t *destPath, 
    BSTR *destPathResult, 
    INT32 *writeAnswer)
{
  UString destPathResultTemp = destPath;
  /*
  {
    CComBSTR destPathResultBSTR = destPath;
    *destPathResult = destPathResultBSTR.Detach();
  }
  */
  *destPathResult = 0;
  *writeAnswer = BoolToMyBool(false);

  UString destPathSpec = destPath;
  CSysString destPathSys = GetSystemString(destPathSpec, _fileCodePage);
  bool srcIsFolderSpec = MyBoolToBool(srcIsFolder);
  CFileInfo aDestFileInfo;
  if (FindFile(destPathSys, aDestFileInfo))
  {
    if (srcIsFolderSpec)
    {
      if (!aDestFileInfo.IsDirectory())
      {
        UString message = UString(L"can not replace file \'")
          + destPathSpec +
          UString(L"\' with folder with same name");
        RETURN_IF_NOT_S_OK(MessageError(message));
        return E_ABORT;
      }
      *writeAnswer = BoolToMyBool(false);
      return S_OK;
    }
    if (aDestFileInfo.IsDirectory())
    {
      UString message = UString(L"can not replace folder \'")
          + destPathSpec +
          UString(L"\' with file with same name");
      RETURN_IF_NOT_S_OK(MessageError(message));
      return E_FAIL;
    }

    switch(_overwriteMode)
    {
      case NExtractionMode::NOverwrite::kSkipExisting:
        return S_OK;
      case NExtractionMode::NOverwrite::kAskBefore:
      {
        INT32 aOverwiteResult;
        RETURN_IF_NOT_S_OK(AskOverwrite(
            destPathSpec, 
            &aDestFileInfo.LastWriteTime, &aDestFileInfo.Size,
            GetUnicodeString(srcPath, _fileCodePage),
            srcTime, aSrcSize, 
            &aOverwiteResult));
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
      if (!AutoRenamePath(destPathSys))
      {
        UString message = UString(L"can not create name of file ")
            + GetUnicodeString(destPathSys, _fileCodePage);
        RETURN_IF_NOT_S_OK(MessageError(message));
        return E_ABORT;
      }
      
      /*
      {
        CComBSTR destPathResultPrev;
        destPathResultBSTR.Attach(*destPathResult);
      }
      */
      // CComBSTR destPathResultBSTR = GetUnicodeString(destPathSys, _fileCodePage);
      // *destPathResult = destPathResultBSTR.Detach();
      destPathResultTemp = GetUnicodeString(destPathSys, _fileCodePage);
    }
    else
      if (!NFile::NDirectory::DeleteFileAlways(destPathSys))
      {
        UString message = UString(L"can not delete output file ")
            + GetUnicodeString(destPathSys, _fileCodePage);
        RETURN_IF_NOT_S_OK(MessageError(message));
        return E_ABORT;
      }
  }
  CComBSTR destPathResultBSTR = destPathResultTemp;
  *destPathResult = destPathResultBSTR.Detach();
  *writeAnswer = BoolToMyBool(true);
  return S_OK;
}

