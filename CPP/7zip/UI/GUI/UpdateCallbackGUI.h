// UpdateCallbackGUI.h

#ifndef __UPDATE_CALLBACK_GUI_H
#define __UPDATE_CALLBACK_GUI_H

#include "../Common/Update.h"
#include "../FileManager/ProgressDialog2.h"

class CUpdateCallbackGUI: public IUpdateCallbackUI2
{
public:
  // bool StdOutMode;
  bool PasswordIsDefined;
  UString Password;
  bool AskPassword;
  UInt64 NumFiles;

  CUpdateCallbackGUI(): 
      PasswordIsDefined(false),
      AskPassword(false),
      // StdOutMode(false)
      ParentWindow(0)
      {}
  
  ~CUpdateCallbackGUI();
  void Init();

  INTERFACE_IUpdateCallbackUI2(;)

  // HRESULT CloseProgress();

  UStringVector FailedFiles;

  CProgressDialog ProgressDialog;
  HWND ParentWindow;
  void StartProgressDialog(const UString &title)
  {
    ProgressDialog.Create(title, ParentWindow);
  }

  UStringVector Messages;
  int NumArchiveErrors;
  void AddErrorMessage(LPCWSTR message);
  void AddErrorMessage(const wchar_t *name, DWORD systemError);
};

#endif
