// UpdateCallbackGUI.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/Error.h"
#include "Windows/PropVariant.h"

#include "../FileManager/PasswordDialog.h"

#include "UpdateCallbackGUI.h"

using namespace NWindows;

CUpdateCallbackGUI::~CUpdateCallbackGUI() {}

void CUpdateCallbackGUI::Init()
{
  FailedFiles.Clear();
  NumFiles = 0;
}

void CUpdateCallbackGUI::AddErrorMessage(LPCWSTR message)
{
  ProgressDialog->Sync.AddErrorMessage(message);
}

void CUpdateCallbackGUI::AddErrorMessage(const wchar_t *name, DWORD systemError)
{
  AddErrorMessage(
      UString(L"WARNING: ") +
      NError::MyFormatMessageW(systemError) +
      UString(L": ") +
      UString(name));
}

HRESULT CUpdateCallbackGUI::OpenResult(const wchar_t *name, HRESULT result)
{
  if (result != S_OK)
  {
    AddErrorMessage (UString(L"Error: ") + name + UString(L" is not supported archive"));
  }
  return S_OK;
}

HRESULT CUpdateCallbackGUI::StartScanning()
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::CanNotFindError(const wchar_t *name, DWORD systemError)
{
  FailedFiles.Add(name);
  AddErrorMessage(name, systemError);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::FinishScanning()
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::StartArchive(const wchar_t *name, bool /* updating */)
{
  ProgressDialog->Sync.SetTitleFileName(name);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::FinishArchive()
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::CheckBreak()
{
  return ProgressDialog->Sync.ProcessStopAndPause();
}

HRESULT CUpdateCallbackGUI::ScanProgress(UInt64 /* numFolders */, UInt64 numFiles, const wchar_t *path)
{
  ProgressDialog->Sync.SetCurrentFileName(path);
  ProgressDialog->Sync.SetNumFilesTotal(numFiles);
  return ProgressDialog->Sync.ProcessStopAndPause();
}

HRESULT CUpdateCallbackGUI::Finilize()
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetNumFiles(UInt64 numFiles)
{
  ProgressDialog->Sync.SetNumFilesTotal(numFiles);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetTotal(UInt64 total)
{
  ProgressDialog->Sync.SetProgress(total, 0);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetCompleted(const UInt64 *completeValue)
{
  RINOK(CheckBreak());
  if (completeValue != NULL)
    ProgressDialog->Sync.SetPos(*completeValue);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  RINOK(CheckBreak());
  ProgressDialog->Sync.SetRatioInfo(inSize, outSize);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::GetStream(const wchar_t *name, bool /* isAnti */)
{
  ProgressDialog->Sync.SetCurrentFileName(name);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::OpenFileError(const wchar_t *name, DWORD systemError)
{
  FailedFiles.Add(name);
  // if (systemError == ERROR_SHARING_VIOLATION)
  {
    AddErrorMessage(name, systemError);
    return S_FALSE;
  }
  // return systemError;
}

HRESULT CUpdateCallbackGUI::SetOperationResult(Int32 /* operationResult */)
{
  NumFiles++;
  ProgressDialog->Sync.SetNumFilesCur(NumFiles);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
  *password = NULL;
  if (!PasswordIsDefined)
  {
    if (passwordIsDefined == 0 || AskPassword)
    {
      CPasswordDialog dialog;
      ProgressDialog->WaitCreating();
      if (dialog.Create(*ProgressDialog) == IDCANCEL)
        return E_ABORT;
      Password = dialog.Password;
      PasswordIsDefined = true;
    }
  }
  if (passwordIsDefined != 0)
    *passwordIsDefined = BoolToInt(PasswordIsDefined);
  return StringToBstr(Password, password);
}

HRESULT CUpdateCallbackGUI::CryptoGetTextPassword(BSTR *password)
{
  return CryptoGetTextPassword2(NULL, password);
}

/*
It doesn't work, since main stream waits Dialog
HRESULT CUpdateCallbackGUI::CloseProgress()
{
  ProgressDialog->MyClose();
  return S_OK;
}
*/


HRESULT CUpdateCallbackGUI::Open_CheckBreak()
{
  return ProgressDialog->Sync.ProcessStopAndPause();
}

HRESULT CUpdateCallbackGUI::Open_SetTotal(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  // if (numFiles != NULL) ProgressDialog->Sync.SetNumFilesTotal(*numFiles);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::Open_SetCompleted(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  return ProgressDialog->Sync.ProcessStopAndPause();
}

#ifndef _NO_CRYPTO

HRESULT CUpdateCallbackGUI::Open_CryptoGetTextPassword(BSTR *password)
{
  PasswordWasAsked = true;
  return CryptoGetTextPassword2(NULL, password);
}

HRESULT CUpdateCallbackGUI::Open_GetPasswordIfAny(UString &password)
{
  if (PasswordIsDefined)
    password = Password;
  return S_OK;
}

bool CUpdateCallbackGUI::Open_WasPasswordAsked()
{
  return PasswordWasAsked;
}

void CUpdateCallbackGUI::Open_ClearPasswordWasAskedFlag()
{
  PasswordWasAsked = false;
}

/*
HRESULT CUpdateCallbackGUI::ShowDeleteFile(const wchar_t *name)
{
  ProgressDialog->Sync.SetCurrentFileName(name);
  return S_OK;
}
*/

#endif
