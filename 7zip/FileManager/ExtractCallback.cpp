// ExtractCallback.h

#include "StdAfx.h"

#include "ExtractCallback.h"

#include "Windows/FileFind.h"
#include "Windows/FileDir.h"

#include "Resource/OverwriteDialog/OverwriteDialog.h"
#include "Resource/PasswordDialog/PasswordDialog.h"
#include "Resource/MessagesDialog/MessagesDialog.h"
#include "../UI/Resource/Extract/resource.h"

#include "Common/WildCard.h"
#include "Common/StringConvert.h"

#include "FormatUtils.h"

#include "../Common/FilePathAutoRename.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

CExtractCallbackImp::~CExtractCallbackImp()
{
  if (!_messages.IsEmpty())
  {
    CMessagesDialog messagesDialog;
    messagesDialog._messages = &_messages;
    messagesDialog.Create(_parentWindow);
  }
}

void CExtractCallbackImp::Init(
    NExtractionMode::NOverwrite::EEnum overwriteMode,
    bool passwordIsDefined, 
    const UString &password)
{
  _overwriteMode = overwriteMode;
  _passwordIsDefined = passwordIsDefined;
  _password = password;
  _messages.Clear();
}

void CExtractCallbackImp::AddErrorMessage(LPCTSTR message)
{
  _messages.Add(message);
}

STDMETHODIMP CExtractCallbackImp::SetTotal(UINT64 total)
{
  ProgressDialog.ProgressSynch.SetProgress(total, 0);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompleted(const UINT64 *completeValue)
{
  while(true)
  {
    if(ProgressDialog.ProgressSynch.GetStopped())
      return E_ABORT;
    if(!ProgressDialog.ProgressSynch.GetPaused())
      break;
    ::Sleep(100);
  }
  if (completeValue != NULL)
    ProgressDialog.ProgressSynch.SetPos(*completeValue);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UINT64 *existSize,
    const wchar_t *newName, const FILETIME *newTime, const UINT64 *newSize,
    INT32 *answer)
{
  COverwriteDialog dialog;

  dialog.OldFileInfo.Time = *existTime;
  if (dialog.OldFileInfo.SizeIsDefined = (existSize != NULL))
    dialog.OldFileInfo.Size = *existSize;
  dialog.OldFileInfo.Name = existName;

  if (newTime == 0)
    dialog.NewFileInfo.TimeIsDefined = false;
  else
  {
    dialog.NewFileInfo.TimeIsDefined = true;
    dialog.NewFileInfo.Time = *newTime;
  }
  
  if (dialog.NewFileInfo.SizeIsDefined = (newSize != NULL))
    dialog.NewFileInfo.Size = *newSize;
  dialog.NewFileInfo.Name = newName;
  
  /*
  NOverwriteDialog::NResult::EEnum writeAnswer = 
    NOverwriteDialog::Execute(oldFileInfo, newFileInfo);
  */
  int writeAnswer = dialog.Create(NULL); // _parentWindow doesn't work with 7z
  
  switch(writeAnswer)
  {
  case IDCANCEL:
    return E_ABORT;
    // askResult = NAskOverwriteAnswer::kCancel;
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


STDMETHODIMP CExtractCallbackImp::PrepareOperation(const wchar_t *name, INT32 askExtractMode)
{
  _currentFilePath = name;
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

STDMETHODIMP CExtractCallbackImp::SetOperationResult(INT32 operationResult)
{
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT messageID;
      UINT32 langID;
      switch(operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          messageID = IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_UNSUPPORTED_METHOD;
          langID = 0x02000A91;
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          messageID = IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_DATA_ERROR;
          langID = 0x02000A92;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
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
  CMyComBSTR tempName = _password;
  *password = tempName.Detach();

  return S_OK;
}


// IExtractCallBack3
STDMETHODIMP CExtractCallbackImp::AskWrite(
    const wchar_t *srcPath, INT32 srcIsFolder, 
    const FILETIME *srcTime, const UINT64 *srcSize,
    const wchar_t *destPath, 
    BSTR *destPathResult, 
    INT32 *writeAnswer)
{
  UString destPathResultTemp = destPath;
  /*
  {
    CMyComBSTR destPathResultBSTR = destPath;
    *destPathResult = destPathResultBSTR.Detach();
  }
  */
  *destPathResult = 0;
  *writeAnswer = BoolToInt(false);

  UString destPathSpec = destPath;
  UString destPathSys = destPathSpec;
  bool srcIsFolderSpec = IntToBool(srcIsFolder);
  CFileInfoW destFileInfo;
  if (FindFile(destPathSys, destFileInfo))
  {
    if (srcIsFolderSpec)
    {
      if (!destFileInfo.IsDirectory())
      {
        UString message = UString(L"can not replace file \'")
          + destPathSpec +
          UString(L"\' with folder with same name");
        RINOK(MessageError(message));
        return E_ABORT;
      }
      *writeAnswer = BoolToInt(false);
      return S_OK;
    }
    if (destFileInfo.IsDirectory())
    {
      UString message = UString(L"can not replace folder \'")
          + destPathSpec +
          UString(L"\' with file with same name");
      RINOK(MessageError(message));
      return E_FAIL;
    }

    switch(_overwriteMode)
    {
      case NExtractionMode::NOverwrite::kSkipExisting:
        return S_OK;
      case NExtractionMode::NOverwrite::kAskBefore:
      {
        INT32 overwiteResult;
        RINOK(AskOverwrite(
            destPathSpec, 
            &destFileInfo.LastWriteTime, &destFileInfo.Size,
            srcPath,
            srcTime, srcSize, 
            &overwiteResult));
          switch(overwiteResult)
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
            + destPathSys;
        RINOK(MessageError(message));
        return E_ABORT;
      }
      destPathResultTemp = destPathSys;
    }
    else
      if (!NFile::NDirectory::DeleteFileAlways(destPathSys))
      {
        UString message = UString(L"can not delete output file ")
            + destPathSys;
        RINOK(MessageError(message));
        return E_ABORT;
      }
  }
  CMyComBSTR destPathResultBSTR = destPathResultTemp;
  *destPathResult = destPathResultBSTR.Detach();
  *writeAnswer = BoolToInt(true);
  return S_OK;
}

