// CompressDialog.cpp

#include "StdAfx.h"

#include "resource.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/IntToString.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/ResourceString.h"

#include "../../FileManager/HelpUtils.h"
#include "../../FileManager/SplitUtils.h"

#include "../Common/ZipRegistry.h"

#include "CompressDialog.h"

#ifdef LANG        
#include "../../FileManager/LangUtils.h"
#endif

#include "../Resource/CompressDialog/resource.h"

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
  { IDC_COMPRESS_SOLID, 0x02000D05 },
  { IDC_COMPRESS_MULTI_THREAD, 0x02000D09 },
  { IDC_STATIC_COMPRESS_VOLUME, 0x02000D40 },
  { IDC_STATIC_COMPRESS_PARAMETERS, 0x02000D06 },
  { IDC_STATIC_COMPRESS_UPDATE_MODE, 0x02000D02 },
  { IDC_STATIC_COMPRESS_OPTIONS, 0x02000D07 },
  { IDC_COMPRESS_SFX, 0x02000D08 },
  { IDC_COMPRESS_PASSWORD, 0x02000802 },
  { IDC_COMPRESS_CHECK_SHOW_PASSWORD, 0x02000B02 },
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
  kBZip2,
  kDeflate
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
    false, false, false, false, false
  },
  { 
    k7zFormat, 
    (1 << 0) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9), 
    g_7zMethods, MY_SIZE_OF_ARRAY(g_7zMethods),
    true, true, true, true, true
  },
  { 
    L"Zip", 
    (1 << 0) | (1 << 5) | (1 << 9), 
    g_ZipMethods, MY_SIZE_OF_ARRAY(g_ZipMethods) ,
    false, false, false, true, false
  },
  { 
    L"GZip", 
    (1 << 5) | (1 << 9), 
    g_GZipMethods, MY_SIZE_OF_ARRAY(g_GZipMethods),
    false, false, false, false, false
  },
  { 
    L"BZip2", 
    (1 << 5), 
    g_BZip2Methods, 
    MY_SIZE_OF_ARRAY(g_BZip2Methods),
    false, false, false, false, false
  },
  { 
    L"Tar", 
    (1 << 0), 
    0, 0,
    false, false, false, false, false
  }
};

static bool IsMethodSupportedBySfx(int methodID)
{
  for (int i = 0; i < MY_SIZE_OF_ARRAY(g_7zSfxMethods); i++)
    if (methodID == g_7zSfxMethods[i])
      return true;
  return false;
};

class CDoubleZeroStringList
{
  CRecordVector<int> m_Indexes;
  CSysString m_String;
public:
  void Add(LPCTSTR s);
  void SetForBuffer(LPTSTR buffer);
};

const TCHAR kDelimiterSymbol = TEXT(' ');
void CDoubleZeroStringList::Add(LPCTSTR s)
{
  m_String += s;
  m_Indexes.Add(m_String.Length());
  m_String += kDelimiterSymbol;
}

void CDoubleZeroStringList::SetForBuffer(LPTSTR buffer)
{
  lstrcpy(buffer, m_String);
  for (int i = 0; i < m_Indexes.Size(); i++)
    buffer[m_Indexes[i]] = TEXT('\0');
}


bool CCompressDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000D00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, MY_SIZE_OF_ARRAY(kIDLangPairs) );
  #endif
  _passwordControl.Attach(GetItem(IDC_COMPRESS_EDIT_PASSWORD));
  _passwordControl.SetText(Info.Password);

  m_ArchivePath.Attach(GetItem(IDC_COMPRESS_COMBO_ARCHIVE));
	m_Format.Attach(GetItem(IDC_COMPRESS_COMBO_FORMAT));
	m_Level.Attach(GetItem(IDC_COMPRESS_COMBO_LEVEL));
	m_Method.Attach(GetItem(IDC_COMPRESS_COMBO_METHOD));
  m_Dictionary.Attach(GetItem(IDC_COMPRESS_COMBO_DICTIONARY));
  m_Order.Attach(GetItem(IDC_COMPRESS_COMBO_ORDER));

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
    const CArchiverInfo &ai = m_ArchiverInfoList[i];
    m_Format.AddString(GetSystemString(ai.Name));
    if (ai.Name.CollateNoCase(
        m_RegistryInfo.ArchiveType) == 0)
      Info.ArchiverInfoIndex = i;
  }
  m_Format.SetCurSel(Info.ArchiverInfoIndex);

  SetArchiveName(Info.ArchiveName);
  SetLevel();
  SetParams();
  
  for(i = 0; i < m_RegistryInfo.HistoryArchives.Size() && i < kHistorySize; i++)
    m_ArchivePath.AddString(m_RegistryInfo.HistoryArchives[i]);

  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_ADD, 0x02000DA1));
  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_UPDATE, 0x02000DA2));
  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_FRESH, 0x02000DA3));
  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_SYNCHRONIZE, 0x02000DA4));

  m_UpdateMode.SetCurSel(0);

  Info.Solid = m_RegistryInfo.Solid;
  Info.MultiThread = m_RegistryInfo.MultiThread;

  CheckButton(IDC_COMPRESS_SOLID, Info.Solid);
  CheckButton(IDC_COMPRESS_MULTI_THREAD, Info.MultiThread);
  CheckButton(IDC_COMPRESS_SFX, Info.SFXMode);
  
  CheckControlsEnable();

  OnButtonSFX();

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
  _passwordControl.SetPasswordChar((IsButtonChecked(
      IDC_COMPRESS_CHECK_SHOW_PASSWORD) == BST_CHECKED) ? 0: TEXT('*'));
  CSysString password;
  _passwordControl.GetText(password);
  _passwordControl.SetText(password);
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

static bool IsMultiProcessor()
{
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  return systemInfo.dwNumberOfProcessors > 1;
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

void CCompressDialog::CheckControlsEnable()
{
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  Info.SolidIsAllowed = fi.Solid;
  bool multiThreadEnable = fi.MultiThread & IsMultiProcessor();
  Info.MultiThreadIsAllowed = multiThreadEnable;
  Info.EncryptHeadersIsAllowed = fi.EncryptFileNames;
  
  EnableItem(IDC_COMPRESS_SOLID, fi.Solid);
  EnableItem(IDC_COMPRESS_MULTI_THREAD, multiThreadEnable);
  CheckSFXControlsEnable();
  
  // EnableItem(IDC_STATIC_COMPRESS_VOLUME, enable);
  // EnableItem(IDC_COMPRESS_COMBO_VOLUME, enable);
  
  EnableItem(IDC_COMPRESS_CHECK_ENCRYPT_FILE_NAMES, fi.EncryptFileNames);
  EnableItem(IDC_COMPRESS_PASSWORD, fi.Encrypt);
  EnableItem(IDC_COMPRESS_EDIT_PASSWORD, fi.Encrypt);
  EnableItem(IDC_COMPRESS_CHECK_SHOW_PASSWORD, fi.Encrypt);
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

  m_Volume.Enable(!isSFX);
  if (isSFX)
    m_Volume.SetText(TEXT(""));
}

void CCompressDialog::OnButtonSetArchive() 
{
  const int kBufferSize = MAX_PATH * 2;
  TCHAR buffer[kBufferSize];
  UString fileName;
  m_ArchivePath.GetText(fileName);
  fileName.TrimLeft();
  fileName.TrimRight();
  Info.ArchiveName = fileName;
  UString fullFileName;
  if (!Info.GetFullPathName(fullFileName))
  {
    fullFileName = Info.ArchiveName;
    // throw "Incorrect archive path";
    return;
  }
  lstrcpy(buffer, GetSystemString(fullFileName));

  OPENFILENAME info;
  info.lStructSize = sizeof(info); 
  info.hwndOwner = HWND(*this); 
  info.hInstance = 0; 
  

  const int kFilterBufferSize = MAX_PATH;
  TCHAR filterBuffer[kFilterBufferSize];
  CDoubleZeroStringList doubleZeroStringList;
  // doubleZeroStringList.Add(TEXT("Zip Files (*.zip)"));
  // doubleZeroStringList.Add(TEXT("*.zip"));
  UString s = LangLoadStringW(IDS_OPEN_TYPE_ALL_FILES, 0x02000DB1);
  s += L" (*.*)";
  doubleZeroStringList.Add(GetSystemString(s));
  doubleZeroStringList.Add(TEXT("*.*"));
  doubleZeroStringList.SetForBuffer(filterBuffer);
  info.lpstrFilter = filterBuffer; 
  
  
  info.lpstrCustomFilter = NULL; 
  info.nMaxCustFilter = 0; 
  info.nFilterIndex = 0; 
  
  info.lpstrFile = buffer; 
  info.nMaxFile = kBufferSize;
  
  info.lpstrFileTitle = NULL; 
    info.nMaxFileTitle = 0; 
  
  info.lpstrInitialDir= NULL; 

  CSysString title = LangLoadString(IDS_COMPRESS_SET_ARCHIVE_DIALOG_TITLE, 0x02000D90);
  
  info.lpstrTitle = title;

  info.Flags = OFN_EXPLORER | OFN_HIDEREADONLY; 
  info.nFileOffset = 0; 
  info.nFileExtension = 0; 
  info.lpstrDefExt = NULL; 
  
  info.lCustData = 0; 
  info.lpfnHook = NULL; 
  info.lpTemplateName = NULL; 

  if(!GetOpenFileName(&info))
    return;
  m_ArchivePath.SetText(buffer);
}

// in ExtractDialog.cpp
extern void AddUniqueString(CSysStringVector &strings, const CSysString &srcString);


void CCompressDialog::OnOK() 
{
  _passwordControl.GetText(Info.Password);

  SaveOptionsInMem();
  int currentItem = m_ArchivePath.GetCurSel();
  UString s;
  if(currentItem == CB_ERR)
  {
    m_ArchivePath.GetText(s);
    if(m_ArchivePath.GetCount() >= kHistorySize)
      currentItem = m_ArchivePath.GetCount() - 1;
  }
  else
  {
    CSysString sTemp;
    m_ArchivePath.GetLBText(currentItem, sTemp);
    s = GetUnicodeString(sTemp);
  }
  s.Trim();
  m_RegistryInfo.HistoryArchives.Clear();
  AddUniqueString(m_RegistryInfo.HistoryArchives, GetSystemString(s));
  Info.ArchiveName = s;
  Info.UpdateMode = NCompressDialog::NUpdateMode::EEnum(m_UpdateMode.GetCurSel());

  Info.Level = GetLevelSpec();
  Info.Dictionary = GetDictionarySpec();
  Info.Order = GetOrderSpec();
  Info.OrderMode = GetOrderMode();
  Info.Method = GetUnicodeString(GetMethodSpec());

  Info.ArchiverInfoIndex = m_Format.GetCurSel();

  Info.SFXMode = IsSFX();
  m_RegistryInfo.Solid = Info.Solid = IsButtonCheckedBool(IDC_COMPRESS_SOLID);
  m_RegistryInfo.MultiThread = Info.MultiThread = IsButtonCheckedBool(IDC_COMPRESS_MULTI_THREAD);
  m_RegistryInfo.EncryptHeaders = Info.EncryptHeaders = IsButtonCheckedBool(IDC_COMPRESS_CHECK_ENCRYPT_FILE_NAMES);

  m_Params.GetText(Info.Options);
  UString volumeString;
  m_Volume.GetText(volumeString);
  volumeString.Trim();
  Info.VolumeSizes.Clear();
  if (!volumeString.IsEmpty())
    if (!ParseVolumeSizes(volumeString, Info.VolumeSizes))
    {
      MessageBox(*this, TEXT("Incorrect volume size"), TEXT("7-Zip"), 0);
      return;
    }

  for(int i = 0; i < m_ArchivePath.GetCount(); i++)
    if(i != currentItem)
    {
      CSysString sTemp;
      m_ArchivePath.GetLBText(i, sTemp);
      sTemp.Trim();
      AddUniqueString(m_RegistryInfo.HistoryArchives, sTemp);
    }
  
  ////////////////////
  // Method

  m_RegistryInfo.Level = Info.Level;
  m_RegistryInfo.ArchiveType = m_ArchiverInfoList[Info.ArchiverInfoIndex].Name;

  m_RegistryInfo.ShowPassword = (IsButtonChecked(
      IDC_COMPRESS_CHECK_SHOW_PASSWORD) == BST_CHECKED);

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
        OnChangeFormat();
        return true;
      }
      case IDC_COMPRESS_COMBO_LEVEL:
      {
        const CArchiverInfo &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
        int index = FindRegistryFormatAlways(ai.Name);
        NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
        fo.Init();
        SetMethod();
        CheckSFXNameChange();
        return true;
      }
      case IDC_COMPRESS_COMBO_METHOD:
      {
        SetDictionary();
        SetOrder();
        CheckSFXNameChange();
        return true;
      }
      case IDC_COMPRESS_COMBO_DICTIONARY:
      case IDC_COMPRESS_COMBO_ORDER:
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
  const CArchiverInfo &prevArchiverInfo = m_ArchiverInfoList[m_PrevFormat];
  if (prevArchiverInfo.KeepName || Info.KeepName)
  {
    UString prevExtension = prevArchiverInfo.GetMainExtension();
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

void CCompressDialog::OnChangeFormat() 
{
  bool isSFX = IsSFX();
  SaveOptionsInMem();
  SetLevel();
  SetParams();
  CheckControlsEnable();
  SetArchiveName2(isSFX);
}

// if type.KeepName then use OriginalFileName
// else if !KeepName remove extension
// add new extension

void CCompressDialog::SetArchiveName(const UString &name)
{
  UString fileName = name;
  Info.ArchiverInfoIndex = m_Format.GetCurSel();
  const CArchiverInfo &ai = m_ArchiverInfoList[Info.ArchiverInfoIndex];
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
      if (dotPos > slashPos)
        fileName = fileName.Left(dotPos);
    }
  }

  if (IsSFX())
    fileName += kExeExt;
  else
  {
    fileName += L'.';
    fileName += ai.GetMainExtension();
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
  const CArchiverInfo &ai = m_ArchiverInfoList[formatIndex];
  for (int i = 0; i < MY_SIZE_OF_ARRAY(g_Formats); i++)
    if (ai.Name.CompareNoCase(g_Formats[i].Name) == 0)
      return i;
  return 0; // -1;
}

void CCompressDialog::SetNearestSelectComboBox(
    NControl::CComboBox &comboBox, UInt32 value)
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
  const CArchiverInfo &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
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
      int index = m_Level.AddString(LangLoadString(levelInfo.ResourceID, levelInfo.LangID));
      m_Level.SetItemData(index, i);
    }
  }
  SetNearestSelectComboBox(m_Level, level);
  SetMethod();
}

int CCompressDialog::GetLevel()
{
  if (m_Level.GetCount() <= 0)
    return -1;
  return m_Level.GetItemData(m_Level.GetCurSel());
}

int CCompressDialog::GetLevelSpec()
{
  if (m_Level.GetCount() <= 1)
    return -1;
  return GetLevel();
}

int CCompressDialog::GetLevel2()
{
  int level = GetLevel();
  if (level < 0)
    level = 5;
  return level;
}

void CCompressDialog::SetMethod() 
{
  m_Method.ResetContent();
  if (GetLevel() <= 0)
  {
    SetDictionary();
    SetOrder();
    return;
  }
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  const CArchiverInfo &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  int index = FindRegistryFormat(ai.Name);
  UString defaultMethod;
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    defaultMethod = GetUnicodeString(fo.Method); 
  }
  bool isSfx = IsSFX();
  for(int m = 0; m < fi.NumMethods; m++)
  {
    EMethodID methodID = fi.MathodIDs[m];
    if (isSfx)
      if (!IsMethodSupportedBySfx(methodID))
        continue;
    const LPCWSTR method = kMethodsNames[methodID];
    int itemIndex = m_Method.AddString(GetSystemString(method));
    if (defaultMethod.CompareNoCase(method) == 0 || m == 0)
      m_Method.SetCurSel(itemIndex);
  }
  SetDictionary();
  SetOrder();
}

int CCompressDialog::GetMethodID()
{
  CSysString methodSpec;
  m_Method.GetText(methodSpec);
  UString methodName = GetUnicodeString(methodSpec);
  for (int i = 0; i < MY_SIZE_OF_ARRAY(kMethodsNames); i++)
    if (methodName.CompareNoCase(kMethodsNames[i]) == 0)
      return i;
  return -1;
}

CSysString CCompressDialog::GetMethodSpec()
{
  if (m_Method.GetCount() <= 1)
    return CSysString();
  CSysString result;
  m_Method.GetText(result);
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
  int index = m_Dictionary.AddString(s);
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
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  const CArchiverInfo &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  int index = FindRegistryFormat(ai.Name);
  UInt32 defaultDictionary = UInt32(-1);
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    if (fo.Method.CollateNoCase(GetMethodSpec()) == 0)
      defaultDictionary = fo.Dictionary;
  }
  int methodID = GetMethodID();
  int level = GetLevel2();
  if (methodID < 0)
  {
    SetMemoryUsage();
    return;
  }
  switch (methodID)
  {
    case kLZMA:
    {
      if (defaultDictionary == UInt32(-1))
      {
        if (level >= 9)
          defaultDictionary = (32 << 20);
        else if (level >= 7)
          defaultDictionary = (8 << 20);
        else if (level >= 5)
          defaultDictionary = (2 << 20);
        else
          defaultDictionary = (32 << 10);
      }
      int i;
      AddDictionarySize(32 << 10);
      for (i = 20; i < 28; i++)
        for (int j = 0; j < 2; j++)
        {
          if (i == 20 && j > 0)
            continue;
          UInt32 dictionary = (1 << i) + (j << (i - 1));
          AddDictionarySize(dictionary);
        }
      SetNearestSelectComboBox(m_Dictionary, defaultDictionary);
      break;
    }
    case kPPMd:
    {
      if (defaultDictionary == UInt32(-1))
      {
        if (level >= 9)
          defaultDictionary = (192 << 20);
        else if (level >= 7)
          defaultDictionary = (64 << 20);
        else if (level >= 5)
          defaultDictionary = (24 << 20);
        else
          defaultDictionary = (4 << 20);
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
      AddDictionarySize(900 << 10);
      m_Dictionary.SetCurSel(0);
      break;
    }
  }
  SetMemoryUsage();
}

UInt32 CCompressDialog::GetDictionary()
{
  if (m_Dictionary.GetCount() <= 0)
    return -1;
  return m_Dictionary.GetItemData(m_Dictionary.GetCurSel());
}

UInt32 CCompressDialog::GetDictionarySpec()
{
  if (m_Dictionary.GetCount() <= 1)
    return -1;
  return GetDictionary();
}

int CCompressDialog::AddOrder(UInt32 size)
{
  TCHAR s[40];
  ConvertUInt64ToString(size, s);
  int index = m_Order.AddString(s);
  m_Order.SetItemData(index, size);
  return index;
}

void CCompressDialog::SetOrder()
{
  m_Order.ResetContent();
  const CFormatInfo &fi = g_Formats[GetStaticFormatIndex()];
  const CArchiverInfo &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
  int index = FindRegistryFormat(ai.Name);
  UInt32 defaultOrder = UInt32(-1);
  if (index >= 0)
  {
    const NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
    if (fo.Method.CollateNoCase(GetMethodSpec()) == 0)
      defaultOrder = fo.Order;
  }
  int methodID = GetMethodID();
  int level = GetLevel2();
  if (methodID < 0)
  {
    SetMemoryUsage();
    return;
  }
  switch (methodID)
  {
    case kLZMA:
    {
      if (defaultOrder == UInt32(-1))
      {
        if (level >= 7)
          defaultOrder = 64;
        else
          defaultOrder = 32;
      }
      int i;
      for (i = 3; i < 8; i++)
        for (int j = 0; j < 2; j++)
        {
          UInt32 order = (1 << i) + (j << (i - 1));
          if (order < 255)
            AddOrder(order);
        }
      AddOrder(255);
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
        if (level >= 7)
          defaultOrder = 64;
        else
          defaultOrder = 32;
      }
      int i;
      for (i = 3; i < 8; i++)
        for (int j = 0; j < 2; j++)
        {
          UInt32 order = (1 << i) + (j << (i - 1));
          if (order < 255)
            AddOrder(order);
        }
      AddOrder(255);
      SetNearestSelectComboBox(m_Order, defaultOrder);
      break;
    }
    case kBZip2:
    {
      break;
    }
  }
  SetMemoryUsage();
}

bool CCompressDialog::GetOrderMode()
{
  switch (GetMethodID())
  {
    case kLZMA:
    case kDeflate:
    case kDeflate64:
      return false;
    case kPPMd:
      return true;
  }
  return false;
}

UInt32 CCompressDialog::GetOrder()
{
  if (m_Order.GetCount() <= 0)
    return -1;
  return m_Order.GetItemData(m_Order.GetCurSel());
}

UInt32 CCompressDialog::GetOrderSpec()
{
  if (m_Order.GetCount() <= 1)
    return -1;
  return GetOrder();
}

UInt64 CCompressDialog::GetMemoryUsage(UInt64 &decompressMemory)
{
  decompressMemory = UInt64(Int64(-1));
  UInt32 dictionary = GetDictionary();
  int level = GetLevel2();
  if (level == 0)
  {
    decompressMemory = (1 << 20);
    return decompressMemory;
  }
  switch (GetMethodID())
  {
    case kLZMA:
    {
      UInt64 size;
      if (level >= 5)
      {
        size = ((UInt64)dictionary * 19 / 2) + (2 << 20);
        if (level >= 9)
          size += (34 << 20) + (12 << 20) * 2 + (5 << 20);
        else
          size += (6 << 20);
      }
      else 
        size = ((UInt64)dictionary * 11 / 2) + (2 << 20);
      decompressMemory = dictionary + (2 << 20);
      return size;
    }
    case kPPMd:
    {
      decompressMemory = dictionary + (2 << 20);
      return decompressMemory;
    }
    case kDeflate:
    case kDeflate64:
    {
      UInt32 order = GetOrder();
      if (order == UInt32(-1))
        order = 32;
      UInt64 size = 0;
      if (level >= 7)
        size = (order * 2 + 4) * (64 << 10);
      size += 3 << 20;
      decompressMemory = (2 << 20);
      return size;
    }
    case kBZip2:
    {
      decompressMemory = (7 << 20);
      return 10 << 20;
    }
  }
  return UInt64(Int64(-1));
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
  const CArchiverInfo &ai = m_ArchiverInfoList[m_Format.GetCurSel()];
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
  const CArchiverInfo &ai = m_ArchiverInfoList[Info.ArchiverInfoIndex];
  int index = FindRegistryFormatAlways(ai.Name);
  m_Params.GetText(Info.Options);
  Info.Options.Trim();
  NCompression::CFormatOptions &fo = m_RegistryInfo.FormatOptionsVector[index];
  fo.Options = GetSystemString(Info.Options);
  fo.Level = GetLevelSpec();
  fo.Dictionary = GetDictionarySpec();
  fo.Order = GetOrderSpec();
  fo.Method = GetMethodSpec();
}
