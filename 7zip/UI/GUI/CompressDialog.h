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
  struct CInfo
  {
    NUpdateMode::EEnum UpdateMode;
    bool SolidIsAllowed;
    bool Solid;

    bool MultiThreadIsAllowed;
    bool MultiThread;

    bool VolumeSizeIsDefined;
    UINT64 VolumeSize;

    UINT32 Level;
    UString Method;
    UINT32 Dictionary;
    bool OrderMode;
    UINT32 Order;
    CSysString Options;

    bool SFXMode;
    
    UString ArchiveName; // in: Relative for ; out: abs
    UString CurrentDirPrefix;
    bool KeepName;

    bool GetFullPathName(UString &result) const;

    int ArchiverInfoIndex;

    void Init() 
    { 
      Level = Dictionary = Order = UINT32(-1); 
      OrderMode = false;
      Method.Empty();
      Options.Empty();
    }
    CInfo()
    {
      Init();
    }
  };
}

class CCompressDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CComboBox	m_ArchivePath;
	NWindows::NControl::CComboBox	m_Format;
	NWindows::NControl::CComboBox	m_Level;
	NWindows::NControl::CComboBox	m_Method;
	NWindows::NControl::CComboBox	m_Dictionary;
	NWindows::NControl::CComboBox	m_Order;
	NWindows::NControl::CComboBox	m_UpdateMode;
	NWindows::NControl::CComboBox	m_Volume;
  NWindows::NControl::CDialogChildControl m_Params;

  NWindows::NControl::CEdit _passwordControl;


  NCompression::CInfo m_RegistryInfo;

  int m_PrevFormat;
  void SetArchiveName(const UString &name);
  int FindRegistryFormat(const UString &name);
  
  void OnChangeFormat();
  
  int GetStaticFormatIndex();

  void SetNearestSelectComboBox(
      NWindows::NControl::CComboBox &comboBox, UINT32 value);

  void SetLevel();
  int GetLevel();
  int GetLevelSpec();
  int GetLevel2();
  
  void SetMethod();
  int GetMethodID();
  CSysString GetMethodSpec();

  AddDictionarySize(UINT32 size, bool kilo, bool maga);
  AddDictionarySize(UINT32 size);
  
  void SetDictionary();
  UINT32 GetDictionary();
  UINT32 GetDictionarySpec();

  int AddOrder(UINT32 size);
  void SetOrder();
  bool GetOrderMode();
  UINT32 GetOrder();
  UINT32 GetOrderSpec();

  UINT64 GetMemoryUsage(UINT64 &decompressMemory);
  void PrintMemUsage(UINT res, UINT64 value);
  void SetMemoryUsage();
  void SetParams();
  void SaveOptionsInMem();

  void UpdatePasswordControl();
public:
  CObjectVector<CArchiverInfo> m_ArchiverInfoList;

  NCompressDialog::CInfo m_Info;
  UString OriginalFileName; // for bzip2, gzip2

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

};

#endif
