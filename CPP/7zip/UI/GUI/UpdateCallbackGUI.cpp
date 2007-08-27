// UpdateCallbackGUI.cpp

#include "StdAfx.h"

#include "UpdateCallbackGUI.h"

#include "Common/StringConvert.h"
#include "Common/IntToString.h"
#include "Common/Defs.h"

#include "Windows/PropVariant.h"
#include "Windows/Error.h"
#include "../FileManager/MessagesDialog.h"
#include "../FileManager/PasswordDialog.h"

using namespace NWindows;

CUpdateCallbackGUI::~CUpdateCallbackGUI()
{
  if (!Messages.IsEmpty())
  {
    CMessagesDialog messagesDialog;
    messagesDialog.Messages = &Messages;
    messagesDialog.Create(ParentWindow);
  }
}

void CUpdateCallbackGUI::Init()
{
  FailedFiles.Clear();
  Messages.Clear();
  NumArchiveErrors = 0;
  NumFiles = 0;
}

void CUpdateCallbackGUI::AddErrorMessage(LPCWSTR message)
{
  Messages.Add(message);
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
    AddErrorMessage (UString(L"Error: ") + name +
        UString(L" is not supported archive"));
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
  ProgressDialog.ProgressSynch.SetTitleFileName(name);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::FinishArchive()
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::CheckBreak()
{
  for (;;)
  {
    if(ProgressDialog.ProgressSynch.GetStopped())
      return E_ABORT;
    if(!ProgressDialog.ProgressSynch.GetPaused())
      break;
    ::Sleep(100);
  }
  return S_OK;
}

HRESULT CUpdateCallbackGUI::Finilize()
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetNumFiles(UInt64 numFiles)
{
  ProgressDialog.ProgressSynch.SetNumFilesTotal(numFiles);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetTotal(UInt64 total)
{
  ProgressDialog.ProgressSynch.SetProgress(total, 0);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetCompleted(const UInt64 *completeValue)
{
  RINOK(CheckBreak());
  if (completeValue != NULL)
    ProgressDialog.ProgressSynch.SetPos(*completeValue);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
{
  RINOK(CheckBreak());
  ProgressDialog.ProgressSynch.SetRatioInfo(inSize, outSize);
  return S_OK;
}

HRESULT CUpdateCallbackGUI::GetStream(const wchar_t *name, bool /* isAnti */)
{
  ProgressDialog.ProgressSynch.SetCurrentFileName(name);
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
  ProgressDialog.ProgressSynch.SetNumFilesCur(NumFiles);
  return S_OK;  
}

HRESULT CUpdateCallbackGUI::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
  if (!PasswordIsDefined) 
  {
    if (AskPassword)
    {
      CPasswordDialog dialog;
      if (dialog.Create(ParentWindow) == IDCANCEL)
        return E_ABORT;
      Password = dialog.Password;
      PasswordIsDefined = true;
    }
  }
  *passwordIsDefined = BoolToInt(PasswordIsDefined);
  CMyComBSTR tempName(Password);
  *password = tempName.Detach();
  return S_OK;
}

/*
It doesn't work, since main stream waits Dialog
HRESULT CUpdateCallbackGUI::CloseProgress() 
{ 
  ProgressDialog.MyClose(); 
  return S_OK;
};
*/