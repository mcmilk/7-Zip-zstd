// ExtractDialog.h

#pragma once

#ifndef __EXTRACTDIALOG_H
#define __EXTRACTDIALOG_H

#include "resource.h"

#include "Windows/Control/Dialog.h"
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
    NFilesMode::EEnum FilesMode;
    CSysStringVector FileList;
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
  NWindows::NControl::CDialogChildControl _passwordControl;
  #endif

	int		_pathMode;
	int		_overwriteMode;

  #ifndef _SFX
  int GetPathNameMode() const;
  int GetOverwriteMode() const;
  int GetFilesMode() const;
  #endif
  
  void OnButtonSetPath();

  virtual bool OnInit();
  virtual bool OnButtonClicked(int aButtonID, HWND aButtonHWND);
  virtual void OnOK();
  #ifndef  NO_REGISTRY
  virtual void OnHelp();
  #endif
	// void UpdateWildCardState();
public:
  bool _enableSelectedFilesButton;
  bool _enableFilesButton;
  CSysString _directoryPath;
	NExtractionDialog::NFilesMode::EEnum _filesMode;
  CSysString _password;

  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_EXTRACT), aWndParent); }
  bool Init(const CSysString &aFileName);
  void GetModeInfo(NExtractionDialog::CModeInfo &aModeInfo); 
};

#endif
