// ExtractDialog.h

#ifndef __EXTRACT_DIALOG_H
#define __EXTRACT_DIALOG_H

#include "ExtractDialogRes.h"

#include "Windows/Control/Edit.h"
#include "Windows/Control/ComboBox.h"

#ifndef  NO_REGISTRY
#include "../Common/ZipRegistry.h"
#endif
#include "../Common/ExtractMode.h"

#include "../FileManager/DialogSize.h"

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
  NWindows::NControl::CComboBox _pathMode;
  NWindows::NControl::CComboBox _overwriteMode;
  #endif

  #ifndef _SFX
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
  {
    #ifdef _SFX
    BIG_DIALOG_SIZE(240, 64);
    #else
    BIG_DIALOG_SIZE(300, 160);
    #endif
    return CModalDialog::Create(SIZED_DIALOG(IDD_DIALOG_EXTRACT), aWndParent);
  }
};

#endif
