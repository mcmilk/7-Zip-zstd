// ExtractDialog.h

#ifndef __EXTRACTDIALOG_H
#define __EXTRACTDIALOG_H

#include "ExtractDialogRes.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/Edit.h"
#include "Windows/Control/ComboBox.h"

#ifndef  NO_REGISTRY
#include "../Common/ZipRegistry.h"
#endif
#include "../Common/ExtractMode.h"

namespace NExtractionDialog
{
  /*
  namespace NFilesMode
  {
    enum EEnum
    {
      kSelected,
      kAll,
      kSpecified
    };
  }
  */
}

class CExtractDialog: public NWindows::NControl::CModalDialog
{
  #ifdef NO_REGISTRY
  NWindows::NControl::CDialogChildControl _path;
  #else
  NWindows::NControl::CComboBox _path;
  #endif
  
  #ifndef _SFX
  NWindows::NControl::CEdit _passwordControl;
  #endif

  #ifndef _SFX
  void GetPathMode();
  void SetPathMode();
  void GetOverwriteMode();
  void SetOverwriteMode();
  // int GetFilesMode() const;
  void UpdatePasswordControl();
  #endif
  
  void OnButtonSetPath();

  virtual bool OnInit();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  virtual void OnOK();
  #ifndef  NO_REGISTRY
  virtual void OnHelp();
  #endif
public:
  // bool _enableSelectedFilesButton;
  // bool _enableFilesButton;
  // NExtractionDialog::NFilesMode::EEnum FilesMode;

  UString DirectoryPath;
  #ifndef _SFX
  UString Password;
  #endif
  NExtract::NPathMode::EEnum PathMode;
  NExtract::NOverwriteMode::EEnum OverwriteMode;

  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(IDD_DIALOG_EXTRACT, aWndParent); }
};

#endif
