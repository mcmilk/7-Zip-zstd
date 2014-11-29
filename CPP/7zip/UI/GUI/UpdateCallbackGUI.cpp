// UpdateCallbackGUI.cpp

#include "StdAfx.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/StringConvert.h"

#include "../../../Windows/PropVariant.h"

#include "../FileManager/PasswordDialog.h"
#include "../FileManager/LangUtils.h"

#include "../FileManager/resourceGUI.h"

#include "resource2.h"

#include "UpdateCallbackGUI.h"

using namespace NWindows;

CUpdateCallbackGUI::~CUpdateCallbackGUI() {}

void CUpdateCallbackGUI::Init()
{
  FailedFiles.Clear();
  NumFiles = 0;
}

HRESULT CUpdateCallbackGUI::OpenResult(const wchar_t *name, HRESULT result, const wchar_t *errorArcType)
{
  if (result != S_OK)
  {
    UString s;
    if (errorArcType)
    {
      s += L"Can not open the file as [";
      s += errorArcType;
      s += L"] archive";
    }
    else
      s += L"The file is not supported archive";
    ProgressDialog->Sync.AddError_Message_Name(s, name);
  }
  return S_OK;
}

HRESULT CUpdateCallbackGUI::StartScanning()
{
  CProgressSync &sync = ProgressDialog->Sync;
  sync.Set_Status(LangString(IDS_SCANNING));
  return S_OK;
}

HRESULT CUpdateCallbackGUI::CanNotFindError(const wchar_t *name, DWORD systemError)
{
  FailedFiles.Add(name);
  ProgressDialog->Sync.AddError_Code_Name(systemError, name);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::FinishScanning()
{
  CProgressSync &sync = ProgressDialog->Sync;
  sync.Set_Status(L"");
  return S_OK;
}

HRESULT CUpdateCallbackGUI::StartArchive(const wchar_t *name, bool /* updating */)
{
  CProgressSync &sync = ProgressDialog->Sync;
  sync.Set_Status(LangString(IDS_PROGRESS_COMPRESSING));
  sync.Set_TitleFileName(name);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::FinishArchive()
{
  CProgressSync &sync = ProgressDialog->Sync;
  sync.Set_Status(L"");
  return S_OK;
}

HRESULT CUpdateCallbackGUI::CheckBreak()
{
  return ProgressDialog->Sync.CheckStop();
}

HRESULT CUpdateCallbackGUI::ScanProgress(UInt64 /* numFolders */, UInt64 numFiles, UInt64 totalSize, const wchar_t *path, bool isDir)
{
  return ProgressDialog->Sync.ScanProgress(numFiles, totalSize, path, isDir);
}

HRESULT CUpdateCallbackGUI::Finilize()
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetNumFiles(UInt64 numFiles)
{
  ProgressDialog->Sync.Set_NumFilesTotal(numFiles);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetTotal(UInt64 total)
{
  ProgressDialog->Sync.Set_NumBytesTotal(total);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetCompleted(const UInt64 *completed)
{
  return ProgressDialog->Sync.Set_NumBytesCur(completed);
}

HRESULT CUpdateCallbackGUI::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  ProgressDialog->Sync.Set_Ratio(inSize, outSize);
  return CheckBreak();
}

HRESULT CUpdateCallbackGUI::GetStream(const wchar_t *name, bool /* isAnti */)
{
  ProgressDialog->Sync.Set_FilePath(name);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::OpenFileError(const wchar_t *name, DWORD systemError)
{
  FailedFiles.Add(name);
  // if (systemError == ERROR_SHARING_VIOLATION)
  {
    ProgressDialog->Sync.AddError_Code_Name(systemError, name);
    return S_FALSE;
  }
  // return systemError;
}

HRESULT CUpdateCallbackGUI::SetOperationResult(Int32 /* operationResult */)
{
  NumFiles++;
  ProgressDialog->Sync.Set_NumFilesCur(NumFiles);
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
      if (dialog.Create(*ProgressDialog) != IDOK)
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
  return ProgressDialog->Sync.CheckStop();
}

HRESULT CUpdateCallbackGUI::Open_SetTotal(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  // if (numFiles != NULL) ProgressDialog->Sync.SetNumFilesTotal(*numFiles);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::Open_SetCompleted(const UInt64 * /* numFiles */, const UInt64 * /* numBytes */)
{
  return ProgressDialog->Sync.CheckStop();
}

#ifndef _NO_CRYPTO

HRESULT CUpdateCallbackGUI::Open_CryptoGetTextPassword(BSTR *password)
{
  PasswordWasAsked = true;
  return CryptoGetTextPassword2(NULL, password);
}

HRESULT CUpdateCallbackGUI::Open_GetPasswordIfAny(bool &passwordIsDefined, UString &password)
{
  passwordIsDefined = PasswordIsDefined;
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
