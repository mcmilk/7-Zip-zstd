// CompressDialog.h

#ifndef __COMPRESS_DIALOG_H
#define __COMPRESS_DIALOG_H

#include "Windows/Control/ComboBox.h"
#include "Windows/Control/Edit.h"

#include "../Common/LoadCodecs.h"
#include "../Common/ZipRegistry.h"

#include "../FileManager/DialogSize.h"

#include "CompressDialogRes.h"

namespace NCompressDialog
{
  namespace NUpdateMode
  {
    enum EEnum
    {
      kAdd,
      kUpdate,
      kFresh,
      kSynchronize
    };
  }
  struct CInfo
  {
    NUpdateMode::EEnum UpdateMode;
    bool SolidIsSpecified;
    bool MultiThreadIsAllowed;
    UInt64 SolidBlockSize;
    UInt32 NumThreads;

    CRecordVector<UInt64> VolumeSizes;

    UInt32 Level;
    UString Method;
    UInt32 Dictionary;
    bool OrderMode;
    UInt32 Order;
    UString Options;

    UString EncryptionMethod;

    bool SFXMode;
    bool OpenShareForWrite;

    
    UString ArchiveName; // in: Relative for ; out: abs
    UString CurrentDirPrefix;
    bool KeepName;

    bool GetFullPathName(UString &result) const;

    int FormatIndex;

    UString Password;
    bool EncryptHeadersIsAllowed;
    bool EncryptHeaders;

    void Init()
    {
      Level = Dictionary = Order = UInt32(-1);
      OrderMode = false;
      Method.Empty();
      Options.Empty();
      EncryptionMethod.Empty();
    }
    CInfo()
    {
      Init();
    }
  };
}

class CCompressDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CComboBox m_ArchivePath;
  NWindows::NControl::CComboBox m_Format;
  NWindows::NControl::CComboBox m_Level;
  NWindows::NControl::CComboBox m_Method;
  NWindows::NControl::CComboBox m_Dictionary;
  NWindows::NControl::CComboBox m_Order;
  NWindows::NControl::CComboBox m_Solid;
  NWindows::NControl::CComboBox m_NumThreads;
  NWindows::NControl::CComboBox m_UpdateMode;
  NWindows::NControl::CComboBox m_Volume;
  NWindows::NControl::CDialogChildControl m_Params;

  NWindows::NControl::CEdit _password1Control;
  NWindows::NControl::CEdit _password2Control;
  NWindows::NControl::CComboBox _encryptionMethod;

  NCompression::CInfo m_RegistryInfo;

  int m_PrevFormat;
  void SetArchiveName(const UString &name);
  int FindRegistryFormat(const UString &name);
  int FindRegistryFormatAlways(const UString &name);
  
  void CheckSFXNameChange();
  void SetArchiveName2(bool prevWasSFX);
  
  int GetStaticFormatIndex();

  void SetNearestSelectComboBox(NWindows::NControl::CComboBox &comboBox, UInt32 value);

  void SetLevel();
  
  void SetMethod(int keepMethodId = -1);
  int GetMethodID();
  UString GetMethodSpec();
  UString GetEncryptionMethodSpec();

  bool IsZipFormat();

  void SetEncryptionMethod();

  int AddDictionarySize(UInt32 size, bool kilo, bool maga);
  int AddDictionarySize(UInt32 size);
  
  void SetDictionary();

  UInt32 GetComboValue(NWindows::NControl::CComboBox &c, int defMax = 0);

  UInt32 GetLevel()  { return GetComboValue(m_Level); }
  UInt32 GetLevelSpec()  { return GetComboValue(m_Level, 1); }
  UInt32 GetLevel2();
  UInt32 GetDictionary() { return GetComboValue(m_Dictionary); }
  UInt32 GetDictionarySpec() { return GetComboValue(m_Dictionary, 1); }
  UInt32 GetOrder() { return GetComboValue(m_Order); }
  UInt32 GetOrderSpec() { return GetComboValue(m_Order, 1); }
  UInt32 GetNumThreadsSpec() { return GetComboValue(m_NumThreads, 1); }
  UInt32 GetNumThreads2() { UInt32 num = GetNumThreadsSpec(); if (num == UInt32(-1)) num = 1; return num; }
  UInt32 GetBlockSizeSpec() { return GetComboValue(m_Solid, 1); }

  int AddOrder(UInt32 size);
  void SetOrder();
  bool GetOrderMode();

  void SetSolidBlockSize();
  void SetNumThreads();

  UInt64 GetMemoryUsage(UInt32 dictionary, UInt64 &decompressMemory);
  UInt64 GetMemoryUsage(UInt64 &decompressMemory);
  void PrintMemUsage(UINT res, UInt64 value);
  void SetMemoryUsage();
  void SetParams();
  void SaveOptionsInMem();

  void UpdatePasswordControl();
  bool IsShowPasswordChecked() const
    { return IsButtonChecked(IDC_COMPRESS_CHECK_SHOW_PASSWORD) == BST_CHECKED; }

  int GetFormatIndex();
public:
  CObjectVector<CArcInfoEx> *ArcFormats;
  CRecordVector<int> ArcIndices;

  NCompressDialog::CInfo Info;
  UString OriginalFileName; // for bzip2, gzip2

  INT_PTR Create(HWND wndParent = 0)
  {
    BIG_DIALOG_SIZE(400, 304);
    return CModalDialog::Create(SIZED_DIALOG(IDD_DIALOG_COMPRESS), wndParent);
  }

protected:

  void CheckSFXControlsEnable();
  void CheckVolumeEnable();
  void CheckControlsEnable();

  void OnButtonSetArchive();
  bool IsSFX();
  void OnButtonSFX();

  virtual bool OnInit();
  virtual bool OnCommand(int code, int itemID, LPARAM lParam);
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  virtual void OnOK();
  virtual void OnHelp();

};

#endif
