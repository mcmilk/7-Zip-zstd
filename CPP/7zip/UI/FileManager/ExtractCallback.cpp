// ExtractCallback.h

#include "StdAfx.h"

#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/Error.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/ResourceString.h"

#include "../../Common/FilePathAutoRename.h"

#include "../GUI/ExtractRes.h"
#include "../GUI/resource.h"

#include "ExtractCallback.h"
#include "FormatUtils.h"
#include "MessagesDialog.h"
#include "OverwriteDialog.h"
#ifndef _NO_CRYPTO
#include "PasswordDialog.h"
#endif

using namespace NWindows;
using namespace NFile;
using namespace NFind;

CExtractCallbackImp::~CExtractCallbackImp()
{
  if (ShowMessages && !Messages.IsEmpty())
  {
    CMessagesDialog messagesDialog;
    messagesDialog.Messages = &Messages;
    messagesDialog.Create(ParentWindow);
  }
}

void CExtractCallbackImp::Init()
{
  Messages.Clear();
  NumArchiveErrors = 0;
  #ifndef _SFX
  NumFolders = NumFiles = 0;
  NeedAddFile = false;
  #endif
}

void CExtractCallbackImp::AddErrorMessage(LPCWSTR message)
{
  Messages.Add(message);
}

STDMETHODIMP CExtractCallbackImp::SetNumFiles(UInt64
  #ifndef _SFX
  numFiles
  #endif
  )
{
  #ifndef _SFX
  ProgressDialog.ProgressSynch.SetNumFilesTotal(numFiles);
  #endif
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetTotal(UInt64 total)
{
  ProgressDialog.ProgressSynch.SetProgress(total, 0);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompleted(const UInt64 *value)
{
  RINOK(ProgressDialog.ProgressSynch.ProcessStopAndPause());
  if (value != NULL)
    ProgressDialog.ProgressSynch.SetPos(*value);
  return S_OK;
}

HRESULT CExtractCallbackImp::Open_CheckBreak()
{
  return ProgressDialog.ProgressSynch.ProcessStopAndPause();
}

HRESULT CExtractCallbackImp::Open_SetTotal(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  // if (numFiles != NULL) ProgressDialog.ProgressSynch.SetNumFilesTotal(*numFiles);
  return S_OK;
}

HRESULT CExtractCallbackImp::Open_SetCompleted(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  RINOK(ProgressDialog.ProgressSynch.ProcessStopAndPause());
  // if (numFiles != NULL) ProgressDialog.ProgressSynch.SetNumFilesCur(*numFiles);
  return S_OK;
}

#ifndef _NO_CRYPTO

HRESULT CExtractCallbackImp::Open_CryptoGetTextPassword(BSTR *password)
{
  return CryptoGetTextPassword(password);
}

HRESULT CExtractCallbackImp::Open_GetPasswordIfAny(UString &password)
{
  if (PasswordIsDefined)
    password = Password;
  return S_OK;
}

bool CExtractCallbackImp::Open_WasPasswordAsked()
{
  return PasswordWasAsked;
}

void CExtractCallbackImp::Open_ClearPasswordWasAskedFlag()
{
  PasswordWasAsked = false;
}

#endif


#ifndef _SFX
STDMETHODIMP CExtractCallbackImp::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  ProgressDialog.ProgressSynch.SetRatioInfo(inSize, outSize);
  return S_OK;
}
#endif

/*
STDMETHODIMP CExtractCallbackImp::SetTotalFiles(UInt64 total)
{
  ProgressDialog.ProgressSynch.SetNumFilesTotal(total);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetCompletedFiles(const UInt64 *value)
{
  if (value != NULL)
    ProgressDialog.ProgressSynch.SetNumFilesCur(*value);
  return S_OK;
}
*/

STDMETHODIMP CExtractCallbackImp::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
    const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
    Int32 *answer)
{
  COverwriteDialog dialog;

  dialog.OldFileInfo.Time = *existTime;
  dialog.OldFileInfo.SizeIsDefined = (existSize != NULL);
  if (dialog.OldFileInfo.SizeIsDefined)
    dialog.OldFileInfo.Size = *existSize;
  dialog.OldFileInfo.Name = existName;

  if (newTime == 0)
    dialog.NewFileInfo.TimeIsDefined = false;
  else
  {
    dialog.NewFileInfo.TimeIsDefined = true;
    dialog.NewFileInfo.Time = *newTime;
  }
  
  dialog.NewFileInfo.SizeIsDefined = (newSize != NULL);
  if (dialog.NewFileInfo.SizeIsDefined)
    dialog.NewFileInfo.Size = *newSize;
  dialog.NewFileInfo.Name = newName;
  
  /*
  NOverwriteDialog::NResult::EEnum writeAnswer =
    NOverwriteDialog::Execute(oldFileInfo, newFileInfo);
  */
  INT_PTR writeAnswer = dialog.Create(ProgressDialog); // ParentWindow doesn't work with 7z
  
  switch(writeAnswer)
  {
    case IDCANCEL: *answer = NOverwriteAnswer::kCancel; return E_ABORT;
    case IDYES: *answer = NOverwriteAnswer::kYes; break;
    case IDNO: *answer = NOverwriteAnswer::kNo; break;
    case IDC_BUTTON_OVERWRITE_YES_TO_ALL: *answer = NOverwriteAnswer::kYesToAll; break;
    case IDC_BUTTON_OVERWRITE_NO_TO_ALL: *answer = NOverwriteAnswer::kNoToAll; break;
    case IDC_BUTTON_OVERWRITE_AUTO_RENAME: *answer = NOverwriteAnswer::kAutoRename; break;
    default: return E_FAIL;
  }
  return S_OK;
}


STDMETHODIMP CExtractCallbackImp::PrepareOperation(const wchar_t *name, bool isFolder, Int32 /* askExtractMode */, const UInt64 * /* position */)
{
  _isFolder = isFolder;
  return SetCurrentFilePath2(name);
}

STDMETHODIMP CExtractCallbackImp::MessageError(const wchar_t *message)
{
  AddErrorMessage(message);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::ShowMessage(const wchar_t *message)
{
  AddErrorMessage(message);
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::SetOperationResult(Int32 operationResult, bool encrypted)
{
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT messageID;
      UInt32 langID;
      switch(operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          messageID = IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_UNSUPPORTED_METHOD;
          langID = 0x02000A91;
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          messageID = encrypted ?
              IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_DATA_ERROR_ENCRYPTED:
              IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_DATA_ERROR;
          langID = encrypted ? 0x02000A94 : 0x02000A92;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          messageID = encrypted ?
              IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_CRC_ENCRYPTED:
              IDS_MESSAGES_DIALOG_EXTRACT_MESSAGE_CRC;
          langID = encrypted ? 0x02000A95 : 0x02000A93;
          break;
        default:
          return E_FAIL;
      }
      if (_needWriteArchivePath)
      {
        AddErrorMessage(_currentArchivePath);
        _needWriteArchivePath = false;
      }
      AddErrorMessage(
        MyFormatNew(messageID,
          #ifdef LANG
          langID,
          #endif
          _currentFilePath));
    }
  }
  #ifndef _SFX
  if (_isFolder)
    NumFolders++;
  else
    NumFiles++;
  ProgressDialog.ProgressSynch.SetNumFilesCur(NumFiles);
  #endif
  return S_OK;
}

////////////////////////////////////////
// IExtractCallbackUI

HRESULT CExtractCallbackImp::BeforeOpen(const wchar_t *name)
{
  #ifndef _SFX
  ProgressDialog.ProgressSynch.SetTitleFileName(name);
  #endif
  _currentArchivePath = name;
  return S_OK;
}

HRESULT CExtractCallbackImp::SetCurrentFilePath2(const wchar_t *path)
{
  _currentFilePath = path;
  #ifndef _SFX
  ProgressDialog.ProgressSynch.SetCurrentFileName(path);
  #endif
  return S_OK;
}

HRESULT CExtractCallbackImp::SetCurrentFilePath(const wchar_t *path)
{
  #ifndef _SFX
  if (NeedAddFile)
    NumFiles++;
  NeedAddFile = true;
  ProgressDialog.ProgressSynch.SetNumFilesCur(NumFiles);
  #endif
  return SetCurrentFilePath2(path);
}

HRESULT CExtractCallbackImp::OpenResult(const wchar_t *name, HRESULT result, bool encrypted)
{
  if (result != S_OK)
  {
    UString message;
    if (result == S_FALSE)
    {
      message = MyFormatNew(encrypted ? IDS_CANT_OPEN_ENCRYPTED_ARCHIVE : IDS_CANT_OPEN_ARCHIVE,
        #ifdef LANG
        (encrypted ? 0x0200060A : 0x02000609),
        #endif
        name);
    }
    else
    {
      message = name;
      message += L": ";
      UString message2;
      if (result == E_OUTOFMEMORY)
        message2 =
        #ifdef LANG
        LangString(IDS_MEM_ERROR, 0x0200060B);
        #else
        MyLoadStringW(IDS_MEM_ERROR);
        #endif
      else
        NError::MyFormatMessage(result, message2);
      message += message2;
    }
    MessageError(message);
    NumArchiveErrors++;
  }
  _currentArchivePath = name;
  _needWriteArchivePath = true;
  return S_OK;
}
  
HRESULT CExtractCallbackImp::ThereAreNoFiles()
{
  return S_OK;
}

HRESULT CExtractCallbackImp::ExtractResult(HRESULT result)
{
  if (result == S_OK)
    return result;
  NumArchiveErrors++;
  if (result == E_ABORT || result == ERROR_DISK_FULL)
    return result;
  MessageError(_currentFilePath);
  MessageError(NError::MyFormatMessageW(result));
  return S_OK;
}

#ifndef _NO_CRYPTO

HRESULT CExtractCallbackImp::SetPassword(const UString &password)
{
  PasswordIsDefined = true;
  Password = password;
  return S_OK;
}

STDMETHODIMP CExtractCallbackImp::CryptoGetTextPassword(BSTR *password)
{
  PasswordWasAsked = true;
  if (!PasswordIsDefined)
  {
    CPasswordDialog dialog;
    if (dialog.Create(ProgressDialog) == IDCANCEL)
      return E_ABORT;
    Password = dialog.Password;
    PasswordIsDefined = true;
  }
  return StringToBstr(Password, password);
}

#endif

// IExtractCallBack3
STDMETHODIMP CExtractCallbackImp::AskWrite(
    const wchar_t *srcPath, Int32 srcIsFolder,
    const FILETIME *srcTime, const UInt64 *srcSize,
    const wchar_t *destPath,
    BSTR *destPathResult,
    Int32 *writeAnswer)
{
  UString destPathResultTemp = destPath;

  // RINOK(StringToBstr(destPath, destPathResult));

  *destPathResult = 0;
  *writeAnswer = BoolToInt(false);

  UString destPathSpec = destPath;
  UString destPathSys = destPathSpec;
  bool srcIsFolderSpec = IntToBool(srcIsFolder);
  CFileInfoW destFileInfo;
  if (destFileInfo.Find(destPathSys))
  {
    if (srcIsFolderSpec)
    {
      if (!destFileInfo.IsDir())
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
    if (destFileInfo.IsDir())
    {
      UString message = UString(L"can not replace folder \'")
          + destPathSpec +
          UString(L"\' with file with same name");
      RINOK(MessageError(message));
      return E_FAIL;
    }

    switch(OverwriteMode)
    {
      case NExtract::NOverwriteMode::kSkipExisting:
        return S_OK;
      case NExtract::NOverwriteMode::kAskBefore:
      {
        Int32 overwiteResult;
        RINOK(AskOverwrite(
            destPathSpec,
            &destFileInfo.MTime, &destFileInfo.Size,
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
            OverwriteMode = NExtract::NOverwriteMode::kSkipExisting;
            return S_OK;
          case NOverwriteAnswer::kYesToAll:
            OverwriteMode = NExtract::NOverwriteMode::kWithoutPrompt;
            break;
          case NOverwriteAnswer::kYes:
            break;
          case NOverwriteAnswer::kAutoRename:
            OverwriteMode = NExtract::NOverwriteMode::kAutoRename;
            break;
          default:
            return E_FAIL;
        }
      }
    }
    if (OverwriteMode == NExtract::NOverwriteMode::kAutoRename)
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
  *writeAnswer = BoolToInt(true);
  return StringToBstr(destPathResultTemp, destPathResult);
}
