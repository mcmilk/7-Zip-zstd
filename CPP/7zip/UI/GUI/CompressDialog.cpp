// CompressDialog.cpp

#include "StdAfx.h"

#include "resource.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/IntToString.h"
#include "Windows/CommonDialog.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/ResourceString.h"
#include "Windows/System.h"

#include "../FileManager/HelpUtils.h"
#include "../FileManager/SplitUtils.h"
#include "../FileManager/FormatUtils.h"

#include "../Explorer/MyMessages.h"

#include "../Common/ZipRegistry.h"

#include "CompressDialog.h"

#ifndef _UNICODE
extern bool g_IsNT;
#endif

#ifdef LANG        
#include "../FileManager/LangUtils.h"
#endif

#include "CompressDialogRes.h"

#define MY_SIZE_OF_ARRAY(x) (sizeof(x) / sizeof(x[0]))

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDC_STATIC_COMPRESS_ARCHIVE, 0x02000D01 },
  { IDC_STATIC_COMPRESS_FORMAT, 0x02000D03 },
  { IDC_STATIC_COMPRESS_LEVEL, 0x02000D0B },
  { IDC_STATIC_COMPRESS_METHOD, 0x02000D04 },
  { IDC_STATIC_COMPRESS_DICTIONARY, 0x02000D0C },
  { IDC_STATIC_COMPRESS_ORDER, 0x02000D0D },
  { IDC_STATIC_COMPRESS_MEMORY, 0x02000D0E },
  { IDC_STATIC_COMPRESS_MEMORY_DE, 0x02000D0F },
  { IDC_STATIC_COMPRESS_THREADS, 0x02000D12 },
  { IDC_STATIC_COMPRESS_SOLID, 0x02000D13 },
  { IDC_STATIC_COMPRESS_VOLUME, 0x02000D40 },
  { IDC_STATIC_COMPRESS_PARAMETERS, 0x02000D06 },
  
  { IDC_STATIC_COMPRESS_UPDATE_MODE, 0x02000D02 },
  { IDC_STATIC_COMPRESS_OPTIONS, 0x02000D07 },
  { IDC_COMPRESS_SFX, 0x02000D08 },
  { IDC_COMPRESS_SHARED, 0x02000D16 },
  
  { IDC_COMPRESS_ENCRYPTION, 0x02000D10 },
  { IDC_STATIC_COMPRESS_PASSWORD1, 0x02000B01 },
  { IDC_STATIC_COMPRESS_PASSWORD2, 0x02000B03 },
  { IDC_COMPRESS_CHECK_SHOW_PASSWORD, 0x02000B02 },
  { IDC_STATIC_COMPRESS_ENCRYPTION_METHOD, 0x02000D11 },
  { IDC_COMPRESS_CHECK_ENCRYPT_FILE_NAMES, 0x02000D0A },

  { IDOK, 0x02000702 },
  { IDCANCEL, 0x02000710 },
  { IDHELP, 0x02000720 }
};
#endif

using namespace NWindows;
using namespace NFile;
using namespace NName;
using namespace NDirectory;

static const int kHistorySize = 8;

static LPCWSTR kExeExt = L".exe";
static LPCWSTR k7zFormat = L"7z";

struct CLevelInfo
{
  UInt32 ResourceID;
  UInt32 LangID;
};

enum ELevel
{
  kStore = 0,
  kFastest = 1,
  kFast = 3,
  kNormal = 5,
  kMaximum = 7,
  kUltra = 9
};

static const CLevelInfo g_Levels[] = 
{
  { IDS_METHOD_STORE, 0x02000D81 },
  { IDS_METHOD_FASTEST, 0x02000D85 },
  { 0, 0 },
  { IDS_METHOD_FAST, 0x02000D84 },
  { 0, 0 },
  { IDS_METHOD_NORMAL, 0x02000D82 },
  { 0, 0 },
  { IDS_METHOD_MAXIMUM, 0x02000D83 },
  { 0, 0 },
  { IDS_METHOD_ULTRA, 0x02000D86 }
};

enum EMethodID
{
  kCopy,
  kLZMA,
  kPPMd,
  kBZip2,
  kDeflate,
  kDeflate64
};

static const LPCWSTR kMethodsNames[] = 
{
  L"Copy",
  L"LZMA",
  L"PPMd",
  L"BZip2",
  L"Deflate",
  L"Deflate64"
};

static const EMethodID g_7zMethods[] = 
{
  kLZMA,
  kPPMd,
  kBZip2
};

static const EMethodID g_7zSfxMethods[] = 
{
  kCopy,
  kLZMA,
  kPPMd
};

static EMethodID g_ZipMethods[] = 
{
  kDeflate,
  kDeflate64,
  kBZip2
};

static EMethodID g_GZipMethods[] = 
{
  kDeflate
};

static EMethodID g_BZip2Methods[] = 
{
  kBZip2
};

struct CFormatInfo
{
  LPCWSTR Name;
  UInt32 LevelsMask;
  const EMethodID *MathodIDs;
  int NumMethods;
  bool Filter;
  bool Solid;
  bool MultiThread;
  bool SFX;
  bool Encrypt;
  bool EncryptFileNames;
};

static const CFormatInfo g_Formats[] = 
{
  { 
    L"", 
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9), 
    0, 0,
    false, false, false, false, false, false
  },
  { 
    k7zFormat, 
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9), 
    g_7zMethods, MY_SIZE_OF_ARRAY(g_7zMethods),
    true, true, true, true, true, true
  },
  { 
    L"Zip", 
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9), 
    g_ZipMethods, MY_SIZE_OF_ARRAY(g_ZipMethods) ,
    false, false, true, false, true, false
  },
  { 
    L"GZip", 
    (1 << 1) | (1 << 5) | (1 << 7) | (1 << 9), 
    g_GZipMethods, MY_SIZE_OF_ARRAY(g_GZipMethods),
    false, false, false, false, false, false
  },
  { 
    L"BZip2", 
    (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9),
    g_BZip2Methods, 
    MY_SIZE_OF_ARRAY(g_BZip2Methods),
    false, false, true, false, false
  },
  { 
    L"Tar", 
    (1 << 0), 
    0, 0,
    false, false, false, false, false, false
  }
};

static bool IsMethodSupportedBySfx(int methodID)
{
  for (int i = 0; i < MY_SIZE_OF_ARRAY(g_7zSfxMethods); i++)
    if (methodID == g_7zSfxMethods[i])
      return true;
  return false;
};

static UInt64 GetMaxRamSizeForProgram()
{
  UInt64 physSize = NSystem::GetRamSize();
  const UInt64 kMinSysSize = (1 << 24);
  if (physSize <= kMinSysSize)
    physSize = 0;
  else
    physSize -= kMinSysSize;
  const UInt64 kMinUseSize = (1 << 25);
  if (physSize < kMinUseSize)
    physSize = kMinUseSize;
  return physSize;
}

bool CCompressDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000D00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, MY_SIZE_OF_ARRAY(kIDLangPairs) );
  #endif
  _password1Control.Attach(GetItem(IDC_COMPRESS_EDIT_PASSWORD1));
  _password2Control.Attach(GetItem(IDC_COMPRESS_EDIT_PASSWORD2));
  _password1Control.SetText(Info.Password);
  _password2Control.SetText(Info.Password);
  _encryptionMethod.Attach(GetItem(IDC_COMPRESS_COMBO_ENCRYPTION_METHOD));

  m_ArchivePath.Attach(GetItem(IDC_COMPRESS_COMBO_ARCHIVE));
  m_Format.Attach(GetItem(IDC_COMPRESS_COMBO_FORMAT));
  m_Level.Attach(GetItem(IDC_COMPRESS_COMBO_LEVEL));
  m_Method.Attach(GetItem(IDC_COMPRESS_COMBO_METHOD));
  m_Dictionary.Attach(GetItem(IDC_COMPRESS_COMBO_DICTIONARY));
  m_Order.Attach(GetItem(IDC_COMPRESS_COMBO_ORDER));
  m_Solid.Attach(GetItem(IDC_COMPRESS_COMBO_SOLID));
  m_NumThreads.Attach(GetItem(IDC_COMPRESS_COMBO_THREADS));
  
  m_UpdateMode.Attach(GetItem(IDC_COMPRESS_COMBO_UPDATE_MODE));
  m_Volume.Attach(GetItem(IDC_COMPRESS_COMBO_VOLUME));
  m_Params.Attach(GetItem(IDC_COMPRESS_EDIT_PARAMETERS));

  AddVolumeItems(m_Volume);

  ReadCompressionInfo(m_RegistryInfo);
  CheckButton(IDC_COMPRESS_CHECK_SHOW_PASSWORD, m_RegistryInfo.ShowPassword);
  CheckButton(IDC_COMPRESS_CHECK_ENCRYPT_FILE_NAMES, m_RegistryInfo.EncryptHeaders);

  UpdatePasswordControl();

  Info.ArchiverInfoIndex = 0;
  int i;
  for(i = 0; i < m_ArchiverInfoList.Size(); i++)
  {
    const CArcInfoEx &ai = m_ArchiverInfoList[i];
    m_Format.AddString(ai.Name);
    if (ai.Name.CompareNoCase(m_RegistryInfo.ArchiveType) == 0)
      Info.ArchiverInfoIndex = i;
  }
  m_Format.SetCurSel(Info.ArchiverInfoIndex);

  SetArchiveName(Info.ArchiveName);
  SetLevel();
  SetParams();
  
  for(i = 0; i < m_RegistryInfo.HistoryArchives.Size() && i < kHistorySize; i++)
    m_ArchivePath.AddString(m_RegistryInfo.HistoryArchives[i]);

  m_UpdateMode.AddString(LangString(IDS_COMPRESS_UPDATE_MODE_ADD, 0x02000DA1));
  m_UpdateMode.AddString(LangString(IDS_COMPRESS_UPDATE_MODE_UPDATE, 0x02000DA2));
  m_UpdateMode.AddString(LangString(IDS_COMPRESS_UPDATE_MODE_FRESH, 0x02000DA3));
  m_UpdateMode.AddString(LangString(IDS_COMPRESS_UPDATE_MODE_SYNCHRONIZE, 0x02000DA4));

  m_UpdateMode.SetCurSel(0);

  SetSolidBlockSize();
  SetNumThreads();

  TCHAR s[40] = { TEXT('/'), TEXT(' '), 0 };
  ConvertUInt64ToString(NSystem::GetNumberOfProcessors(), s + 2);
  SetItemText(IDC_COMPRESS_HARDWARE_THREADS, s);

  CheckButton(IDC_COMPRESS_SFX, Info.SFXMode);
  CheckButton(IDC_COMPRESS_SHARED, Info.OpenShareForWrite);
  
  CheckControlsEnable();

  OnButtonSFX();

  SetEncryptionMethod();
  SetMemoryUsage();
  return CModalDialog::OnInit();
}

namespace NCompressDialog
{
  bool CInfo::GetFullPathName(UString &result) const
  {
    NDirectory::MySetCurrentDirectory(CurrentDirPrefix);
    return MyGetFullPathName(ArchiveName, result);
  }
}

void CCompressDialog::UpdatePasswordControl()
{
  bool showPassword = IsShowPasswordChecked();
  TCHAR c = showPassword ? 0: TEXT('*');
  _password1Control.SetPasswordChar(c);
  _password2Control.SetPasswordChar(c);
  UString password;
  _password1Control.GetText(password);
  _password1Control.SetText(password);
  _password2Control.GetText(password);
  _password2Control.SetText(password);

  int cmdShow = showPassword ? SW_HIDE : SW_SHOW;
  ShowItem(IDC_STATIC_COMPRESS_PASSWORD2, cmdShow);
  _password2Control.Show(cmdShow);
}

bool CCompressDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDC_COMPRESS_BUTTON_SET_ARCHIVE:
    {
      OnButtonSetArchive();
      return true;
    }
    case IDC_COMPRESS_SFX:
    {
      OnButtonSFX();
      SetMemoryUsage();
      return true;
    }
    case IDC_COMPRESS_CHECK_SHOW_PASSWORD:
    {
      UpdatePasswordControl();
      return true;
    }
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CCompressDialog::CheckSFXControlsEnable()
{
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  bool enable = fi.SFX;
  if (enable)
  {
    int methodID = GetMethodID();
    enable = (methodID == -1 || IsMethodSupportedBySfx(methodID));
  }
  if (!enable)
    CheckButton(IDC_COMPRESS_SFX, false);
  EnableItem(IDC_COMPRESS_SFX, enable);
}

void CCompressDialog::CheckVolumeEnable()
{
  bool isSFX = IsSFX();
  m_Volume.Enable(!isSFX);
  if (isSFX)
    m_Volume.SetText(TEXT(""));
}

void CCompressDialog::CheckControlsEnable()
{
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  Info.SolidIsSpecified = fi.Solid;
  bool multiThreadEnable = fi.MultiThread;
  Info.MultiThreadIsAllowed = multiThreadEnable;
  Info.EncryptHeadersIsAllowed = fi.EncryptFileNames;
  
  EnableItem(IDC_COMPRESS_COMBO_SOLID, fi.Solid);
  EnableItem(IDC_COMPRESS_COMBO_THREADS, multiThreadEnable);
  CheckSFXControlsEnable();
  CheckVolumeEnable();

  EnableItem(IDC_COMPRESS_ENCRYPTION, fi.Encrypt);

  EnableItem(IDC_STATIC_COMPRESS_PASSWORD1, fi.Encrypt);
  EnableItem(IDC_STATIC_COMPRESS_PASSWORD2, fi.Encrypt);
  EnableItem(IDC_COMPRESS_EDIT_PASSWORD1, fi.Encrypt);
  EnableItem(IDC_COMPRESS_EDIT_PASSWORD2, fi.Encrypt);
  EnableItem(IDC_COMPRESS_CHECK_SHOW_PASSWORD, fi.Encrypt);

  EnableItem(IDC_STATIC_COMPRESS_ENCRYPTION_METHOD, fi.Encrypt);
  EnableItem(IDC_COMPRESS_COMBO_ENCRYPTION_METHOD, fi.Encrypt);
  EnableItem(IDC_COMPRESS_CHECK_ENCRYPT_FILE_NAMES, fi.EncryptFileNames);
}

bool CCompressDialog::IsSFX()
{
  CWindow sfxButton = GetItem(IDC_COMPRESS_SFX);
  return sfxButton.IsEnabled() && IsButtonCheckedBool(IDC_COMPRESS_SFX);
}

void CCompressDialog::OnButtonSFX()
{
  SetMethod();

  UString fileName;
  m_ArchivePath.GetText(fileName);
  int dotPos = fileName.ReverseFind(L'.');
  int slashPos = fileName.ReverseFind(L'\\');
  if (dotPos < 0 || dotPos <= slashPos)
    dotPos = -1;
  bool isSFX = IsSFX();
  if (isSFX)
  {
    if (dotPos >= 0)
      fileName = fileName.Left(dotPos);
    fileName += kExeExt;
    m_ArchivePath.SetText(fileName);
  }
  else
  {
    if (dotPos >= 0)
    {
      UString ext = fileName.Mid(dotPos);
      if (ext.CompareNoCase(kExeExt) == 0)
      {
        fileName = fileName.Left(dotPos);
        m_ArchivePath.SetText(fileName);
      }
    }
    SetArchiveName2(false); // it's for OnInit
  }

  CheckVolumeEnable();
}

void CCompressDialog::OnButtonSetArchive() 
{
  UString fileName;
  m_ArchivePath.GetText(fileName);
  fileName.Trim();
  Info.ArchiveName = fileName;
  UString fullFileName;
  if (!Info.GetFullPathName(fullFileName))
  {
    fullFileName = Info.ArchiveName;
    return;
  }
  UString title = LangString(IDS_COMPRESS_SET_ARCHIVE_DIALOG_TITLE, 0x02000D90);
  UString s = LangString(IDS_OPEN_TYPE_ALL_FILES, 0x02000DB1);
  s += L" (*.*)";
  UString resPath;
  if (!MyGetOpenFileName(HWND(*this), title, fullFileName, s, resPath))
    return;
  m_ArchivePath.SetText(resPath);
}

// in ExtractDialog.cpp
extern void AddUniqueString(UStringVector &strings, const UString &srcString);

static bool IsAsciiString(const UString &s)
{
  for (int i = 0; i < s.Length(); i++)
  {
    wchar_t c = s[i];
    if (c < 0x20 || c > 0x7F)
      return false;
  }
  return true;
}

void CCompressDialog::OnOK() 
{
  _password1Control.GetText(Info.Password);
  if (IsZipFormat())
  {
    if (!IsAsciiString(Info.Password))
    {
      MyMessageBoxResource(*this, IDS_PASSWORD_USE_ASCII, 0x02000B11);
      return;
    }
    UString method = GetEncryptionMethodSpec();
    method.MakeUpper();
    if (method.Find(L"AES") == 0)
    {
      if (Info.Password.Length() > 99)
      {
        MyMessageBoxResource(*this, IDS_PASSWORD_IS_TOO_LONG, 0x02000B12);
        return;
      }
    }
  }
  if (!IsShowPasswordChecked())
  {
    UString password2;
    _password2Control.GetText(password2);
    if (password2 != Info.Password)
    {
      MyMessageBoxResource(*this, IDS_PASSWORD_PASSWORDS_DO_NOT_MATCH, 0x02000B10);
      return;
    }
  }

  SaveOptionsInMem();
  UString s;
  m_ArchivePath.GetText(s);
  s.Trim();
  m_RegistryInfo.HistoryArchives.Clear();
  AddUniqueString(m_RegistryInfo.HistoryArchives, s);
  Info.ArchiveName = s;
  Info.UpdateMode = NCompressDialog::NUpdateMode::EEnum(m_UpdateMode.GetCurSel());

  Info.Level = GetLevelSpec();
  Info.Dictionary = GetDictionarySpec();
  Info.Order = GetOrderSpec();
  Info.OrderMode = GetOrderMode();
  Info.NumThreads = GetNumThreadsSpec();

  UInt32 solidLogSize = GetBlockSizeSpec();
  Info.SolidBlockSize = 0;
  if (solidLogSize > 0 && solidLogSize != (UInt32)-1)
    Info.SolidBlockSize = (solidLogSize >= 64) ? (UInt64)(Int64)-1 : ((UInt64)1 << solidLogSize);

  Info.Method = GetMethodSpec();
  Info.EncryptionMethod = GetEncryptionMethodSpec();
  Info.ArchiverInfoIndex = m_Format.GetCurSel();
  Info.SFXMode = IsSFX();
  Info.OpenShareForWrite = IsButtonCheckedBool(IDC_COMPRESS_SHARED);

  m_RegistryInfo.EncryptHeaders = Info.EncryptHeaders = IsButtonCheckedBool(IDC_COMPRESS_CHECK_ENCRYPT_FILE_NAMES);

  m_Params.GetText(Info.Options);
  UString volumeString;
  m_Volume.GetText(volumeString);
  volumeString.Trim();
  Info.VolumeSizes.Clear();
  if (!volumeString.IsEmpty())
  {
    if (!ParseVolumeSizes(volumeString, Info.VolumeSizes))
    {
      MyMessageBoxResource(*this, IDS_COMPRESS_INCORRECT_VOLUME_SIZE, 0x02000D41);
      return;
    }
    if (!Info.VolumeSizes.IsEmpty())
    {
      const UInt64 volumeSize = Info.VolumeSizes.Back();
      if (volumeSize < (100 << 10))
      {
        wchar_t s[32];
        ConvertUInt64ToString(volumeSize, s);
        if (::MessageBoxW(*this, MyFormatNew(IDS_COMPRESS_SPLIT_CONFIRM_MESSAGE, 0x02000D42, s), 
            L"7-Zip", MB_YESNOCANCEL | MB_ICONQUESTION | MB_TASKMODAL) != IDYES)
          return;
      }
    }
  }

  for(int i = 0; i < m_ArchivePath.GetCount(); i++)
  {
    UString sTemp;
    m_ArchivePath.GetLBText(i, sTemp);
    sTemp.Trim();
    AddUniqueString(m_RegistryInfo.HistoryArchives, sTemp);
  }
  if (m_RegistryInfo.HistoryArchives.Size() > kHistorySize)
    m_RegistryInfo.HistoryArchives.DeleteBack();
  
  m_RegistryInfo.ArchiveType = m_ArchiverInfoList[Info.ArchiverInfoIndex].Name;
  m_RegistryInfo.ShowPassword = IsShowPasswordChecked();

  SaveCompressionInfo(m_RegistryInfo);
  
  CModalDialog::OnOK();
}

static LPCWSTR kHelpTopic = L"fm/plugins/7-zip/add.htm";

void CCompressDialog::OnHelp() 
{
  ShowHelpWindow(NULL, kHelpTopic);
}

bool CCompressDialog::OnCommand(int code, int itemID, LPARAM lParam)
{
  if (code == CBN_SELCHANGE)
  {
    switch(itemID)
    {
      case IDC_COMPRESS_COMBO_FORMAT:
      {
        bool isSFX = IsSFX();
        SaveOptionsInMem();
        SetLevel();
        SetSolidBlockSize();
        SetNumThreads();
        SetParams();
        CheckControlsEnable();
        SetArchiveName2(isSFX);
        SetEncryptionMethod();
        SetMemoryUsage();
        return true;
      }
      case IDC_COMPRESS_COMBO_LEVEL:
      {
        const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
        int index = FindRegistryFormatAlways(ai.Name);
        NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
        fo.ResetForLevelChange();
        SetMethod();
        SetSolidBlockSize();
        SetNumThreads();
        CheckSFXNameChange();
        SetMemoryUsage();
        return true;
      }
      case IDC_COMPRESS_COMBO_METHOD:
      {
        SetDictionary();
        SetOrder();
        SetSolidBlockSize();
        SetNumThreads();
        CheckSFXNameChange();
        SetMemoryUsage();
        return true;
      }
      case IDC_COMPRESS_COMBO_DICTIONARY:
      case IDC_COMPRESS_COMBO_ORDER:
      {
        SetSolidBlockSize();
        SetMemoryUsage();
        return true;
      }
      case IDC_COMPRESS_COMBO_THREADS:
      {
        SetMemoryUsage();
        return true;
      }
    }
  }
  return CModalDialog::OnCommand(code, itemID, lParam);
}

void CCompressDialog::CheckSFXNameChange() 
{
  bool isSFX = IsSFX();
  CheckSFXControlsEnable();
  if (isSFX != IsSFX())
    SetArchiveName2(isSFX);
}

void CCompressDialog::SetArchiveName2(bool prevWasSFX) 
{
  UString fileName;
  m_ArchivePath.GetText(fileName);
  const CArcInfoEx &prevArchiverInfo = m_ArchiverInfoList[m_PrevFormat];
  if (prevArchiverInfo.KeepName || Info.KeepName)
  {
    UString prevExtension = prevArchiverInfo.GetMainExt();
    if (prevWasSFX)
      prevExtension = kExeExt;
    else
      prevExtension = UString('.') + prevExtension;
    const int prevExtensionLen = prevExtension.Length();
    if (fileName.Length() >= prevExtensionLen)
      if (fileName.Right(prevExtensionLen).CompareNoCase(prevExtension) == 0)
        fileName = fileName.Left(fileName.Length() - prevExtensionLen);
  }
  SetArchiveName(fileName);
}

// if type.KeepName then use OriginalFileName
// else if !KeepName remove extension
// add new extension

void CCompressDialog::SetArchiveName(const UString &name)
{
  UString fileName = name;
  Info.ArchiverInfoIndex = m_Format.GetCurSel();
  const CArcInfoEx &ai = m_ArchiverInfoList[Info.ArchiverInfoIndex];
  m_PrevFormat = Info.ArchiverInfoIndex;
  if (ai.KeepName)
  {
    fileName = OriginalFileName;
  }
  else
  {
    if (!Info.KeepName)
    {
      int dotPos = fileName.ReverseFind('.');
      int slashPos = MyMax(fileName.ReverseFind('\\'), fileName.ReverseFind('/'));
      if (dotPos >= 0 && dotPos > slashPos + 1)
        fileName = fileName.Left(dotPos);
    }
  }

  if (IsSFX())
    fileName += kExeExt;
  else
  {
    fileName += L'.';
    fileName += ai.GetMainExt();
  }
  m_ArchivePath.SetText(fileName);
}

int CCompressDialog::FindRegistryFormat(const UString &name)
{
  for (int i = 0; i < m_RegistryInfo.FormatOptionsVector.Size(); i++)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[i];
    if (GetUnicodeString(fo.FormatID) == name)
      return i;
  }
  return -1;
}

int CCompressDialog::FindRegistryFormatAlways(const UString &name)
{
  int index = FindRegistryFormat(name);
  if (index < 0)
  {
    NCompression::CFormatOptions fo;
    fo.FormatID = GetSystemString(name);
    index = m_RegistryInfo.FormatOptionsVector.Add(fo);
  }
  return index;
}

int CCompressDialog::GetStaticFormatIndex()
{
  int formatIndex = m_Format.GetCurSel();
  const CArcInfoEx &ai = m_ArchiverInfoList[formatIndex];
  for (int i = 0; i < MY_SIZE_OF_ARRAY(g_Formats); i++)
    if (ai.Name.CompareNoCase(g_Formats[i].Name) == 0)
      return i;
  return 0; // -1;
}

void CCompressDialog::SetNearestSelectComboBox(NControl::CComboBox &comboBox, UInt32 value)
{
  for (int i = comboBox.GetCount() - 1; i >= 0; i--)
    if ((UInt32)comboBox.GetItemData(i) <= value)
    {
      comboBox.SetCurSel(i);
      return;
    }
  if (comboBox.GetCount() > 0)
    comboBox.SetCurSel(0);
}

void CCompressDialog::SetLevel()
{
  m_Level.ResetContent();
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  int index = FindRegistryFormat(ai.Name);
  UInt32 level = kNormal;
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    if (fo.Level <= kUltra)
      level = fo.Level;
    else
      level = kUltra;
  }
  int i;
  for (i = 0; i <= kUltra; i++)
  {
    if ((fi.LevelsMask & (1 << i)) != 0)
    {
      const CLevelInfo &levelInfo = g_Levels[i];
      int index = (int)m_Level.AddString(LangString(levelInfo.ResourceID, levelInfo.LangID));
      m_Level.SetItemData(index, i);
    }
  }
  SetNearestSelectComboBox(m_Level, level);
  SetMethod();
}

void CCompressDialog::SetMethod() 
{
  m_Method.ResetContent();
  UInt32 level = GetLevel();
  if (level == 0)
  {
    SetDictionary();
    SetOrder();
    return;
  }
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  int index = FindRegistryFormat(ai.Name);
  UString defaultMethod;
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    defaultMethod = fo.Method; 
  }
  bool isSfx = IsSFX();
  for(int m = 0; m < fi.NumMethods; m++)
  {
    EMethodID methodID = fi.MathodIDs[m];
    if (isSfx)
      if (!IsMethodSupportedBySfx(methodID))
        continue;
    const LPCWSTR method = kMethodsNames[methodID];
    int itemIndex = (int)m_Method.AddString(GetSystemString(method));
    if (defaultMethod.CompareNoCase(method) == 0 || m == 0)
      m_Method.SetCurSel(itemIndex);
  }
  SetDictionary();
  SetOrder();
}

bool CCompressDialog::IsZipFormat()
{
  const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  return (ai.Name.CompareNoCase(L"zip") == 0);
}

void CCompressDialog::SetEncryptionMethod() 
{
  _encryptionMethod.ResetContent();
  const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  if (ai.Name.CompareNoCase(L"7z") == 0)
  {
    _encryptionMethod.AddString(TEXT("AES-256"));
    _encryptionMethod.SetCurSel(0);
  }
  else if (ai.Name.CompareNoCase(L"zip") == 0)
  {
    int index = FindRegistryFormat(ai.Name);
    UString encryptionMethod;
    if (index >= 0)
    {
      const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
      encryptionMethod = fo.EncryptionMethod; 
    }
    _encryptionMethod.AddString(TEXT("ZipCrypto"));
    _encryptionMethod.AddString(TEXT("AES-256"));
    _encryptionMethod.SetCurSel(encryptionMethod.Find(L"AES") == 0 ? 1 : 0);
  }
}

int CCompressDialog::GetMethodID()
{
  UString methodName;
  m_Method.GetText(methodName);
  for (int i = 0; i < MY_SIZE_OF_ARRAY(kMethodsNames); i++)
    if (methodName.CompareNoCase(kMethodsNames[i]) == 0)
      return i;
  return -1;
}

UString CCompressDialog::GetMethodSpec()
{
  if (m_Method.GetCount() <= 1)
    return UString();
  UString result;
  m_Method.GetText(result);
  return result;
}

UString CCompressDialog::GetEncryptionMethodSpec()
{
  if (_encryptionMethod.GetCount() <= 1)
    return UString();
  if (_encryptionMethod.GetCurSel() <= 0)
    return UString();
  UString result;
  _encryptionMethod.GetText(result);
  result.Replace(L"-", L"");
  return result;
}

int CCompressDialog::AddDictionarySize(UInt32 size, bool kilo, bool maga)
{
  UInt32 sizePrint = size;
  if (kilo)
    sizePrint >>= 10;
  else if (maga)
    sizePrint >>= 20;
  TCHAR s[40];
  ConvertUInt64ToString(sizePrint, s);
  if (kilo)
    lstrcat(s, TEXT(" K"));
  else if (maga)
    lstrcat(s, TEXT(" M"));
  else
    lstrcat(s, TEXT(" "));
  lstrcat(s, TEXT("B"));
  int index = (int)m_Dictionary.AddString(s);
  m_Dictionary.SetItemData(index, size);
  return index;
}

int CCompressDialog::AddDictionarySize(UInt32 size)
{
  if (size > 0)
  {
    if ((size & 0xFFFFF) == 0)
      return AddDictionarySize(size, false, true);
    if ((size & 0x3FF) == 0)
      return AddDictionarySize(size, true, false);
  }
  return AddDictionarySize(size, false, false);
}

void CCompressDialog::SetDictionary()
{
  m_Dictionary.ResetContent();
  const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  int index = FindRegistryFormat(ai.Name);
  UInt32 defaultDictionary = UInt32(-1);
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    if (fo.Method.CompareNoCase(GetMethodSpec()) == 0)
      defaultDictionary = fo.Dictionary;
  }
  int methodID = GetMethodID();
  UInt32 level = GetLevel2();
  if (methodID < 0)
    return;
  const UInt64 maxRamSize = GetMaxRamSizeForProgram();
  switch (methodID)
  {
    case kLZMA:
    {
      static const UInt32 kMinDicSize = (1 << 16);
      if (defaultDictionary == UInt32(-1))
      {
        if (level >= 9)      defaultDictionary = (1 << 26);
        else if (level >= 7) defaultDictionary = (1 << 25);
        else if (level >= 5) defaultDictionary = (1 << 24);
        else if (level >= 3) defaultDictionary = (1 << 20);
        else                 defaultDictionary = (kMinDicSize);
      }
      int i;
      AddDictionarySize(kMinDicSize);
      m_Dictionary.SetCurSel(0);
      for (i = 20; i <= 30; i++)
        for (int j = 0; j < 2; j++)
        {
          if (i == 20 && j > 0)
            continue;
          UInt32 dictionary = (1 << i) + (j << (i - 1));
          if (dictionary >
          #ifdef _WIN64
            (1 << 30)
          #else
            (1 << 26)
          #endif
            )
            continue;
          AddDictionarySize(dictionary);
          UInt64 decomprSize;
          UInt64 requiredComprSize = GetMemoryUsage(dictionary, decomprSize);
          if (dictionary <= defaultDictionary && requiredComprSize <= maxRamSize)
             m_Dictionary.SetCurSel(m_Dictionary.GetCount() - 1);
        }

      // SetNearestSelectComboBox(m_Dictionary, defaultDictionary);
      break;
    }
    case kPPMd:
    {
      if (defaultDictionary == UInt32(-1))
      {
        if (level >= 9)      defaultDictionary = (192 << 20);
        else if (level >= 7) defaultDictionary = ( 64 << 20);
        else if (level >= 5) defaultDictionary = ( 16 << 20);
        else                 defaultDictionary = (  4 << 20);
      }
      int i;
      for (i = 20; i < 31; i++)
        for (int j = 0; j < 2; j++)
        {
          if (i == 20 && j > 0)
            continue;
          UInt32 dictionary = (1 << i) + (j << (i - 1));
          if (dictionary >= (1 << 31))
            continue;
          AddDictionarySize(dictionary);
          UInt64 decomprSize;
          UInt64 requiredComprSize = GetMemoryUsage(dictionary, decomprSize);
          if (dictionary <= defaultDictionary && requiredComprSize <= maxRamSize || m_Dictionary.GetCount() == 0)
             m_Dictionary.SetCurSel(m_Dictionary.GetCount() - 1);
        }
      SetNearestSelectComboBox(m_Dictionary, defaultDictionary);
      break;
    }
    case kDeflate:
    {
      AddDictionarySize(32 << 10);
      m_Dictionary.SetCurSel(0);
      break;
    }
    case kDeflate64:
    {
      AddDictionarySize(64 << 10);
      m_Dictionary.SetCurSel(0);
      break;
    }
    case kBZip2:
    {
      UInt32 defaultDictionary;
      if (level >= 5)
        defaultDictionary = (900 << 10);
      else if (level >= 3)
        defaultDictionary = (500 << 10);
      else
        defaultDictionary = (100 << 10);
      for (int i = 1; i <= 9; i++)
      {
        UInt32 dictionary = (i * 100) << 10;
        AddDictionarySize(dictionary);
        if (dictionary <= defaultDictionary || m_Dictionary.GetCount() == 0)
           m_Dictionary.SetCurSel(m_Dictionary.GetCount() - 1);
      }
      break;
    }
  }
}

UInt32 CCompressDialog::GetComboValue(NWindows::NControl::CComboBox &c, int defMax)
{
  if (c.GetCount() <= defMax)
    return (UInt32)-1;
  return (UInt32)c.GetItemData(c.GetCurSel());
}

UInt32 CCompressDialog::GetLevel2()
{
  UInt32 level = GetLevel();
  if (level == (UInt32)-1)
    level = 5;
  return level;
}

int CCompressDialog::AddOrder(UInt32 size)
{
  TCHAR s[40];
  ConvertUInt64ToString(size, s);
  int index = (int)m_Order.AddString(s);
  m_Order.SetItemData(index, size);
  return index;
}

void CCompressDialog::SetOrder()
{
  m_Order.ResetContent();
  const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  int index = FindRegistryFormat(ai.Name);
  UInt32 defaultOrder = UInt32(-1);
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    if (fo.Method.CompareNoCase(GetMethodSpec()) == 0)
      defaultOrder = fo.Order;
  }
  int methodID = GetMethodID();
  UInt32 level = GetLevel2();
  if (methodID < 0)
    return;
  switch (methodID)
  {
    case kLZMA:
    {
      if (defaultOrder == UInt32(-1))
        defaultOrder = (level >= 7) ? 64 : 32;
      for (int i = 3; i <= 8; i++)
        for (int j = 0; j < 2; j++)
        {
          UInt32 order = (1 << i) + (j << (i - 1));
          if (order <= 256)
            AddOrder(order);
        }
      AddOrder(273);
      SetNearestSelectComboBox(m_Order, defaultOrder);
      break;
    }
    case kPPMd:
    {
      if (defaultOrder == UInt32(-1))
      {
        if (level >= 9)
          defaultOrder = 32;
        else if (level >= 7)
          defaultOrder = 16;
        else if (level >= 5)
          defaultOrder = 6;
        else
          defaultOrder = 4;
      }
      int i;
      AddOrder(2);
      AddOrder(3);
      for (i = 2; i < 8; i++)
        for (int j = 0; j < 4; j++)
        {
          UInt32 order = (1 << i) + (j << (i - 2));
          if (order < 32)
            AddOrder(order);
        }
      AddOrder(32);
      SetNearestSelectComboBox(m_Order, defaultOrder);
      break;
    }
    case kDeflate:
    case kDeflate64:
    {
      if (defaultOrder == UInt32(-1))
      {
        if (level >= 9)
          defaultOrder = 128;
        else if (level >= 7)
          defaultOrder = 64;
        else
          defaultOrder = 32;
      }
      int i;
      for (i = 3; i <= 8; i++)
        for (int j = 0; j < 2; j++)
        {
          UInt32 order = (1 << i) + (j << (i - 1));
          if (order <= 256)
            AddOrder(order);
        }
      AddOrder(methodID == kDeflate64 ? 257 : 258);
      SetNearestSelectComboBox(m_Order, defaultOrder);
      break;
    }
    case kBZip2:
    {
      break;
    }
  }
}

bool CCompressDialog::GetOrderMode()
{
  switch (GetMethodID())
  {
    case kPPMd:
      return true;
  }
  return false;
}

static const UInt32 kNoSolidBlockSize = 0;
static const UInt32 kSolidBlockSize = 64;

void CCompressDialog::SetSolidBlockSize()
{
  m_Solid.ResetContent();
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  if (!fi.Solid)
    return;

  UInt32 level = GetLevel2();
  if (level == 0)
    return;

  UInt32 dictionary = GetDictionarySpec();
  if (dictionary == UInt32(-1))
    dictionary = 1;

  UInt32 defaultBlockSize = (UInt32)-1;

  const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  int index = FindRegistryFormat(ai.Name);
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    if (fo.Method.CompareNoCase(GetMethodSpec()) == 0)
      defaultBlockSize = fo.BlockLogSize;
  }

  index = (int)m_Solid.AddString(LangString(IDS_COMPRESS_NON_SOLID, 0x02000D14));
  m_Solid.SetItemData(index, (UInt32)kNoSolidBlockSize);
  m_Solid.SetCurSel(0);
  bool needSet = defaultBlockSize == (UInt32)-1;
  for (int i = 20; i <= 36; i++)
  {
    if (needSet && dictionary >= (((UInt64)1 << (i - 7))) && i <= 32)
      defaultBlockSize = i;
    TCHAR s[40];
    ConvertUInt64ToString(1 << (i % 10), s);
    if (i < 30) lstrcat(s, TEXT(" M"));
    else        lstrcat(s, TEXT(" G"));
    lstrcat(s, TEXT("B"));
    int index = (int)m_Solid.AddString(s);
    m_Solid.SetItemData(index, (UInt32)i);
  }
  index = (int)m_Solid.AddString(LangString(IDS_COMPRESS_SOLID, 0x02000D15));
  m_Solid.SetItemData(index, kSolidBlockSize);
  if (defaultBlockSize == (UInt32)-1)
    defaultBlockSize = kSolidBlockSize;
  if (defaultBlockSize != kNoSolidBlockSize)
    SetNearestSelectComboBox(m_Solid, defaultBlockSize);
}

void CCompressDialog::SetNumThreads()
{
  m_NumThreads.ResetContent();

  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  if (!fi.MultiThread)
    return;

  UInt32 numHardwareThreads = NSystem::GetNumberOfProcessors();
  UInt32 defaultValue = numHardwareThreads;

  const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  int index = FindRegistryFormat(ai.Name);
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    if (fo.Method.CompareNoCase(GetMethodSpec()) == 0)
      defaultValue = fo.NumThreads;
  }

  UInt32 numAlgoThreadsMax = 1;
  int methodID = GetMethodID();
  switch (methodID)
  {
    case kLZMA:
    {
      numAlgoThreadsMax = 2;
      break;
    }
    case kBZip2:
    {
      numAlgoThreadsMax = 32;
      break;
    }
  }
  if (IsZipFormat())
    numAlgoThreadsMax = 128;
  for (UInt32 i = 1; i <= numHardwareThreads * 2 && i <= numAlgoThreadsMax; i++)
  {
    TCHAR s[40];
    ConvertUInt64ToString(i, s);
    int index = (int)m_NumThreads.AddString(s);
    m_NumThreads.SetItemData(index, (UInt32)i);
  }
  SetNearestSelectComboBox(m_NumThreads, defaultValue);
}


UInt64 CCompressDialog::GetMemoryUsage(UInt32 dictionary, UInt64 &decompressMemory)
{
  decompressMemory = UInt64(Int64(-1));
  UInt32 level = GetLevel2();
  if (level == 0)
  {
    decompressMemory = (1 << 20);
    return decompressMemory;
  }
  UInt64 size = 0;

  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  if (fi.Filter && level >= 9)
    size += (12 << 20) * 2 + (5 << 20);
  UInt32 numThreads = GetNumThreads2();
  if (IsZipFormat())
  {
    if (numThreads > 1)
      size += (UInt64)numThreads << 25;
  }
  switch (GetMethodID())
  {
    case kLZMA:
    {
      UInt32 hs = dictionary - 1;
      hs |= (hs >> 1);
      hs |= (hs >> 2);
      hs |= (hs >> 4);
      hs |= (hs >> 8);
      hs >>= 1;
      hs |= 0xFFFF;
      if (hs > (1 << 24))
        hs >>= 1;
      hs++;
      size += hs * 4;
      size += (UInt64)dictionary * 11 / 2;
      if (level >= 5)
        size += dictionary * 4;
      size += (2 << 20);
      if (numThreads > 1 && level >= 5)
        size += (2 << 20) + (4 << 20);

      decompressMemory = dictionary + (2 << 20);
      return size;
    }
    case kPPMd:
    {
      decompressMemory = dictionary + (2 << 20);
      return size + decompressMemory;
    }
    case kDeflate:
    case kDeflate64:
    {
      UInt32 order = GetOrder();
      if (order == UInt32(-1))
        order = 32;
      if (level >= 7)
        size += (1 << 20);
      size += 3 << 20;
      decompressMemory = (2 << 20);
      return size;
    }
    case kBZip2:
    {
      decompressMemory = (7 << 20);
      UInt64 memForOneThread = (10 << 20);
      return size + memForOneThread * numThreads;
    }
  }
  return UInt64(Int64(-1));
}

UInt64 CCompressDialog::GetMemoryUsage(UInt64 &decompressMemory)
{
  return GetMemoryUsage(GetDictionary(), decompressMemory);
}

void CCompressDialog::PrintMemUsage(UINT res, UInt64 value)
{
  if (value == (UInt64)Int64(-1))
  {
    SetItemText(res, TEXT("?"));
    return;
  }
  value = (value + (1 << 20) - 1) >> 20;
  TCHAR s[40];
  ConvertUInt64ToString(value, s);
  lstrcat(s, TEXT(" MB"));
  SetItemText(res, s);
}
    
void CCompressDialog::SetMemoryUsage()
{
  UInt64 decompressMem; 
  UInt64 memUsage = GetMemoryUsage(decompressMem);
  PrintMemUsage(IDC_STATIC_COMPRESS_MEMORY_VALUE, memUsage);
  PrintMemUsage(IDC_STATIC_COMPRESS_MEMORY_DE_VALUE, decompressMem);
}

void CCompressDialog::SetParams()
{
  const CArcInfoEx &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  m_Params.SetText(TEXT(""));
  int index = FindRegistryFormat(ai.Name);
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    m_Params.SetText(fo.Options);
  }
}

void CCompressDialog::SaveOptionsInMem()
{
  const CArcInfoEx &ai = m_ArchiverInfoList[Info.ArchiverInfoIndex];
  int index = FindRegistryFormatAlways(ai.Name);
  m_Params.GetText(Info.Options);
  Info.Options.Trim();
  NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
  fo.Options = Info.Options;
  fo.Level = GetLevelSpec();
  fo.Dictionary = GetDictionarySpec();
  fo.Order = GetOrderSpec();
  fo.Method = GetMethodSpec();
  fo.EncryptionMethod = GetEncryptionMethodSpec();
  fo.NumThreads = GetNumThreadsSpec();
  fo.BlockLogSize = GetBlockSizeSpec();
}
