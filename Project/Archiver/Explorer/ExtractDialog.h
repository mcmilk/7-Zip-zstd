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
  NWindows::NControl::CDialogChildControl m_Path;
  #else
  NWindows::NControl::CComboBox	m_Path;
  #endif
  
  #ifndef _SFX
  NWindows::NControl::CDialogChildControl m_PasswordControl;
  #endif

	int		m_PathMode;
	int		m_OverwriteMode;

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
  #ifndef  NO_REGISTRY
	CZipRegistryManager *m_ZipRegistryManager;
  #endif
	// void UpdateWildCardState();
public:
  bool m_EnableSelectedFilesButton;
  bool m_EnableFilesButton;
  CSysString m_DirectoryPath;
	NExtractionDialog::NFilesMode::EEnum m_FilesMode;
  CSysString m_Password;

  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_EXTRACT), aWndParent); }
  bool Init(
    #ifndef  NO_REGISTRY
    CZipRegistryManager *aManager, 
    #endif
    const CSysString &aFileName);
  void GetModeInfo(NExtractionDialog::CModeInfo &aModeInfo); 
};

#endif
