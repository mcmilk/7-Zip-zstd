// UpdateCallbackConsole.cpp

#include "StdAfx.h"

#include "UpdateCallbackGUI.h"

#include "Common/StringConvert.h"
#include "Common/IntToString.h"
#include "Common/Defs.h"

#include "Windows/PropVariant.h"
#include "Windows/Error.h"
#include "../../FileManager/Resource/MessagesDialog/MessagesDialog.h"
#include "../../FileManager/Resource/PasswordDialog/PasswordDialog.h"

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
}

void CUpdateCallbackGUI::AddErrorMessage(LPCWSTR message)
{
  Messages.Add(GetSystemString(message));
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

HRESULT CUpdateCallbackGUI::FinishScanning()
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::StartArchive(const wchar_t *name, bool updating)
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::FinishArchive()
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::CheckBreak()
{
  while(true)
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

HRESULT CUpdateCallbackGUI::GetStream(const wchar_t *name, bool isAnti)
{
  return S_OK;
}

HRESULT CUpdateCallbackGUI::OpenFileError(const wchar_t *name, DWORD systemError)
{
  FailedFiles.Add(name);
  if (systemError == ERROR_SHARING_VIOLATION)
  {
    AddErrorMessage(
      UString(L"WARNING: ") + 
      NError::MyFormatMessageW(systemError) + 
      UString(L": ") + 
      UString(name));
    return S_FALSE;
  }
  return systemError;
}

HRESULT CUpdateCallbackGUI::SetOperationResult(Int32 operationResult)
{
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