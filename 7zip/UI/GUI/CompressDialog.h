// CompressDialog.h

#pragma once

#ifndef __COMPRESSDIALOG_H
#define __COMPRESSDIALOG_H

#include "../Common/ZipRegistry.h"
#include "../Common/ArchiverInfo.h"
#include "../Resource/CompressDialog/resource.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/Edit.h"
#include "Windows/Control/ComboBox.h"

namespace NCompressDialog
{
  namespace NUpdateMode
  {
    enum EEnum
    {
      kAdd,
      kUpdate,
      kFresh,
      kSynchronize,
    };
  }
  /*
  namespace NMethod
  {
    enum EEnum
    {
      kStore,
      kFastest,
      kFast,
      kNormal,
      kMaximum,
      kUltra
    };
  }
  */
  struct CInfo
  {
    NUpdateMode::EEnum UpdateMode;
    // NMethod::EEnum Method;
    UINT32 Method; // 0 ... 9
    bool SolidIsAllowed;
    bool Solid;

    bool MultiThreadIsAllowed;
    bool MultiThread;

    bool VolumeSizeIsDefined;
    UINT64 VolumeSize;

    CSysString Options;

    bool SFXMode;
    
    CSysString ArchiveName; // in: Relative for ; out: abs
    CSysString CurrentDirPrefix;
    bool KeepName;

    bool GetFullPathName(CSysString &result) const;

    int ArchiverInfoIndex;
  };
}

class CCompressDialog: public NWindows::NControl::CModalDialog
{
  CSysString ArchiveNameSrc;

  NWindows::NControl::CComboBox	m_ArchivePath;
	NWindows::NControl::CComboBox	m_Format;
	NWindows::NControl::CComboBox	m_Method;
	NWindows::NControl::CComboBox	m_UpdateMode;
	NWindows::NControl::CComboBox	m_Volume;
  NWindows::NControl::CDialogChildControl m_Options;

  NWindows::NControl::CEdit _passwordControl;


  NCompression::CInfo m_RegistryInfo;

  int m_PrevFormat;
  void SetArchiveName(const CSysString &name);
  int FindFormat(const UString &name);
  void SaveOptions();
  void SetOptions();
  void UpdatePasswordControl();
public:
  CObjectVector<CArchiverInfo> m_ArchiverInfoList;

  NCompressDialog::CInfo m_Info;

  CSysString Password;
  bool EncryptHeadersIsAllowed;
  bool EncryptHeaders;

  INT_PTR Create(HWND wndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_COMPRESS ), wndParent); }

protected:

  void CheckControlsEnable();

	void OnButtonSetArchive();
  void OnButtonSFX();

  virtual bool OnInit();
  virtual bool OnCommand(int code, int itemID, LPARAM lParam);
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  virtual void OnOK();
  virtual void OnHelp();

  void OnSelChangeComboFormat();
};

#endif
