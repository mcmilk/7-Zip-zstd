// UpdateCallbackGUI.h

#ifndef __UPDATE_CALLBACK_GUI_H
#define __UPDATE_CALLBACK_GUI_H

#include "../Common/Update.h"
#include "../../FileManager/Resource/ProgressDialog2/ProgressDialog.h"

class CUpdateCallbackGUI: public IUpdateCallbackUI2
{
public:
  // bool StdOutMode;
  bool PasswordIsDefined;
  UString Password;
  bool AskPassword;

  CUpdateCallbackGUI(): 
      PasswordIsDefined(false),
      AskPassword(false),
      // StdOutMode(false)
      ParentWindow(0)
      {}
  
  ~CUpdateCallbackGUI();
  void Init();

  HRESULT OpenResult(const wchar_t *name, HRESULT result);

  HRESULT StartScanning();
  HRESULT FinishScanning();

  HRESULT StartArchive(const wchar_t *name, bool updating);
  HRESULT FinishArchive();

  HRESULT CheckBreak();
  HRESULT Finilize();
  HRESULT SetTotal(UInt64 total);
  HRESULT SetCompleted(const UInt64 *completeValue);

  HRESULT GetStream(const wchar_t *name, bool isAnti);
  HRESULT OpenFileError(const wchar_t *name, DWORD systemError);
  HRESULT SetOperationResult(Int32 operationResult);
  HRESULT CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password);

  // HRESULT CloseProgress();

  UStringVector FailedFiles;

  CProgressDialog ProgressDialog;
  HWND ParentWindow;
  void StartProgressDialog(const UString &title)
  {
    ProgressDialog.Create(title, ParentWindow);
  }

  CSysStringVector Messages;
  int NumArchiveErrors;
  void AddErrorMessage(LPCWSTR message);
};

#endif
