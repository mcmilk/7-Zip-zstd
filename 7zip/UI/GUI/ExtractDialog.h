// ExtractDialog.h

#pragma once

#ifndef __EXTRACTDIALOG_H
#define __EXTRACTDIALOG_H

#include "resource.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/Edit.h"
#include "Windows/Control/ComboBox.h"

#ifndef  NO_REGISTRY
#include "../Common/ZipRegistry.h"
#endif

namespace NExtractionDialog
{
  namespace NFilesMode
  {
    enum EEnum
    {
      kSelected,
      kAll,
      kSpecified
    };
  }
  namespace NPathMode
  {
    enum EEnum
    {
      kFullPathnames,
      kCurrentPathnames,
      kNoPathnames,
    };
  }
  namespace NOverwriteMode
  {
    enum EEnum
    {
      kAskBefore,
      kWithoutPrompt,
      kSkipExisting,
      kAutoRename
    };
  }
  struct CModeInfo
  {
    NOverwriteMode::EEnum OverwriteMode;
    NPathMode::EEnum PathMode;
    // NFilesMode::EEnum FilesMode;
    UStringVector FileList;
  };
}

class CExtractDialog: public NWindows::NControl::CModalDialog
{
  #ifdef NO_REGISTRY
  NWindows::NControl::CDialogChildControl _path;
  #else
  NWindows::NControl::CComboBox	_path;
  #endif
  
  #ifndef _SFX
  NWindows::NControl::CEdit _passwordControl;
  #endif

	int		_pathMode;
	int		_overwriteMode;

  #ifndef _SFX
  int GetPathNameMode() const;
  int GetOverwriteMode() const;
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
  UString DirectoryPath;
	// NExtractionDialog::NFilesMode::EEnum FilesMode;
  UString Password;

  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_EXTRACT), aWndParent); }
  void GetModeInfo(NExtractionDialog::CModeInfo &modeInfo); 
};

#endif
