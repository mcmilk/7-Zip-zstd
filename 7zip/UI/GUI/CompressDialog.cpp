// CompressDialog.cpp

#include "StdAfx.h"

#include "resource.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/ResourceString.h"

#include "../../FileManager/HelpUtils.h"
#include "../Common/ZipRegistry.h"

#include "CompressDialog.h"

#ifdef LANG        
#include "../../FileManager/LangUtils.h"
#endif

#include "../Resource/CompressDialog/resource.h"

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDC_STATIC_COMPRESS_ARCHIVE, 0x02000D01 },
  { IDC_STATIC_COMPRESS_FORMAT, 0x02000D03 },
  { IDC_STATIC_COMPRESS_METHOD, 0x02000D04 },
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

static const kHistorySize = 8;


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
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _passwordControl.Attach(GetItem(IDC_COMPRESS_EDIT_PASSWORD));
  _passwordControl.SetText(TEXT(""));

  m_ArchivePath.Attach(GetItem(IDC_COMPRESS_COMBO_ARCHIVE));
	m_Format.Attach(GetItem(IDC_COMPRESS_COMBO_FORMAT));
	m_Method.Attach(GetItem(IDC_COMPRESS_COMBO_METHOD));
	m_UpdateMode.Attach(GetItem(IDC_COMPRESS_COMBO_UPDATE_MODE));
  m_Volume.Attach(GetItem(IDC_COMPRESS_COMBO_VOLUME));
  m_Options.Attach(GetItem(IDC_COMPRESS_EDIT_PARAMETERS));

  m_Volume.AddString(TEXT("1457664 - 3.5 Floppy"));
  m_Volume.AddString(TEXT("650M - CD-650MB"));
  m_Volume.AddString(TEXT("700M - CD-700MB"));

  ReadCompressionInfo(m_RegistryInfo);
  CheckButton(IDC_COMPRESS_CHECK_SHOW_PASSWORD, m_RegistryInfo.ShowPassword);
  CheckButton(IDC_COMPRESS_CHECK_ENCRYPT_FILE_NAMES, m_RegistryInfo.EncryptHeaders);

  UpdatePasswordControl();

  m_Info.ArchiverInfoIndex = 0;
  for(int i = 0; i < m_ArchiverInfoList.Size(); i++)
  {
    const CArchiverInfo &archiverInfo = m_ArchiverInfoList[i];
    m_Format.AddString(GetSystemString(archiverInfo.Name));
    if (m_RegistryInfo.LastClassIDDefined &&
        archiverInfo.Name.CollateNoCase(
        m_RegistryInfo.LastArchiveType) == 0)
      m_Info.ArchiverInfoIndex = i;
  }
  m_Format.SetCurSel(m_Info.ArchiverInfoIndex);

  ArchiveNameSrc = m_Info.ArchiveName;
  /*
  ArchiveNameSrc2.Empty();
  if (!m_Info.KeepName)
  {
    int dotPos = m_Info.ArchiveName.ReverseFind('.');
    int slashPos = MyMax(m_Info.ArchiveName.ReverseFind('\\'), 
        m_Info.ArchiveName.ReverseFind('/'));
    if (dotPos > slashPos && dotPos >= 0)
    {
      ArchiveNameSrc1 = m_Info.ArchiveName.Left(dotPos);
      ArchiveNameSrc2 = m_Info.ArchiveName.Mid(dotPos);
    }
  }
  */


  SetArchiveName(m_Info.ArchiveName);
  SetOptions();
  
  /*
  m_Info.ArchiveName += L'.';
  m_Info.ArchiveName += m_ArchiverInfoList[m_Info.ArchiverInfoIndex].Extension;
  m_ArchivePath.SetText(m_Info.ArchiveName);
  */

  // m_ArchivePath.AddString(m_ArcPathResult);
  for(i = 0; i < m_RegistryInfo.HistoryArchives.Size() && i < kHistorySize; i++)
    m_ArchivePath.AddString(m_RegistryInfo.HistoryArchives[i]);
  // if(m_RegistryInfo.HistoryArchives.Size() > 0) 
  //  m_ArchivePath.SetCurSel(0);
  // else
  //  m_ArchivePath.SetCurSel(-1);

  m_Method.AddString(LangLoadString(IDS_METHOD_STORE, 0x02000D81));
  m_Method.AddString(LangLoadString(IDS_METHOD_FAST, 0x02000D84));
  m_Method.AddString(LangLoadString(IDS_METHOD_NORMAL, 0x02000D82));
  m_Method.AddString(LangLoadString(IDS_METHOD_MAXIMUM, 0x02000D83));
  
  int methodIndex = NCompressDialog::NMethod::kNormal;
  if (m_RegistryInfo.MethodDefined && m_RegistryInfo.Method <= NCompressDialog::NMethod::kMaximum)
    methodIndex = m_RegistryInfo.Method;
  m_Method.SetCurSel(methodIndex);

  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_ADD, 0x02000DA1));
  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_UPDATE, 0x02000DA2));
  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_FRESH, 0x02000DA3));
  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_SYNCHRONIZE, 0x02000DA4));

  m_UpdateMode.SetCurSel(0);

  m_Info.Solid = m_RegistryInfo.Solid;
  m_Info.MultiThread = m_RegistryInfo.MultiThread;

  CheckButton(IDC_COMPRESS_SOLID, m_Info.Solid);
  CheckButton(IDC_COMPRESS_MULTI_THREAD, m_Info.MultiThread);
  CheckButton(IDC_COMPRESS_SFX, m_Info.SFXMode);
  
  CheckControlsEnable();

  OnButtonSFX();



  return CModalDialog::OnInit();
}

namespace NCompressDialog
{
  bool CInfo::GetFullPathName(CSysString &result) const
  {
    ::SetCurrentDirectory(CurrentDirPrefix);
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

void CCompressDialog::CheckControlsEnable()
{
  int formatIndex = m_Format.GetCurSel();
  const CArchiverInfo &archiverInfo = m_ArchiverInfoList[formatIndex];
  bool enable = (archiverInfo.Name == L"7z");
  m_Info.SolidIsAllowed = enable;
  m_Info.MultiThreadIsAllowed = enable;
  EncryptHeadersIsAllowed = enable;
  CWindow control;
  control = GetItem(IDC_COMPRESS_SOLID);
  control.Enable(enable);
  control = GetItem(IDC_COMPRESS_MULTI_THREAD);
  control.Enable(enable);
  control = GetItem(IDC_COMPRESS_SFX);
  control.Enable(enable);
  control = GetItem(IDC_STATIC_COMPRESS_VOLUME);
  control.Enable(enable);
  control = GetItem(IDC_COMPRESS_COMBO_VOLUME);
  control.Enable(enable);
  
  control = GetItem(IDC_COMPRESS_CHECK_ENCRYPT_FILE_NAMES);
  control.Enable(enable);

  enable = (archiverInfo.Name.CompareNoCase(L"zip") == 0) || 
      (archiverInfo.Name.CompareNoCase(L"7z") == 0);
  control = GetItem(IDC_COMPRESS_PASSWORD);
  control.Enable(enable);
  control = GetItem(IDC_COMPRESS_EDIT_PASSWORD);
  control.Enable(enable);
  control = GetItem(IDC_COMPRESS_CHECK_SHOW_PASSWORD);
  control.Enable(enable);

}

void CCompressDialog::OnButtonSFX()
{
  CWindow sfxButton = GetItem(IDC_COMPRESS_SFX);
  bool sfxMode = sfxButton.IsEnabled() && IsButtonCheckedBool(IDC_COMPRESS_SFX);
  if (sfxMode)
  {
    CSysString fileName;
    m_ArchivePath.GetText(fileName);
    int dotPos = fileName.ReverseFind('.');
    int slashPos = fileName.ReverseFind('\\');
    if (dotPos >= 0 && dotPos > slashPos)
      fileName = fileName.Left(dotPos);
    fileName += TEXT(".exe");
    m_ArchivePath.SetText(fileName);
  }
  else
    OnSelChangeComboFormat();
}

void CCompressDialog::OnButtonSetArchive() 
{
  const kBufferSize = MAX_PATH * 2;
  TCHAR buffer[kBufferSize];
  CSysString fileName;
  m_ArchivePath.GetText(fileName);
  fileName.TrimLeft();
  fileName.TrimRight();
  m_Info.ArchiveName = fileName;
  CSysString fullFileName;
  if (!m_Info.GetFullPathName(fullFileName))
  {
    fullFileName = m_Info.ArchiveName;
    // AfxMessageBox("Incorrect archive path");;
    return;
  }
  lstrcpy(buffer, fullFileName);

  OPENFILENAME info;
  info.lStructSize = sizeof(info); 
  info.hwndOwner = HWND(*this); 
  info.hInstance = 0; 
  

  const kFilterBufferSize = MAX_PATH;
  TCHAR filterBuffer[kFilterBufferSize];
  CDoubleZeroStringList doubleZeroStringList;
  // doubleZeroStringList.Add(TEXT("Zip Files (*.zip)"));
  // doubleZeroStringList.Add(TEXT("*.zip"));
  CSysString s = LangLoadString(IDS_OPEN_TYPE_ALL_FILES, 0x02000DB1);
  s += TEXT(" (*.*)");
  doubleZeroStringList.Add(s);
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

bool ParseVolumeSize(const CSysString &s, UINT64 &value)
{
  const TCHAR *start = s;
  const TCHAR *end;
  value = ConvertStringToUINT64(start, &end);
  if (start == end)
    return false;
  while (true)
  {
    TCHAR c = *end++;
    c = MyCharUpper(c);
    switch(c)
    {
      case TEXT('\0'):
      case TEXT('B'):
        return true;
      case TEXT('K'):
        value <<= 10;
        return true;
      case TEXT('M'):
        value <<= 20;
        return true;
      case TEXT('G'):
        value <<= 30;
        return true;
      case TEXT(' '):
        continue;
      default:
        return true;
    }
  }
}

void CCompressDialog::OnOK() 
{
  _passwordControl.GetText(Password);

  SaveOptions();
  int currentItem = m_ArchivePath.GetCurSel();
  CSysString s;
  if(currentItem == CB_ERR)
  {
    m_ArchivePath.GetText(s);
    if(m_ArchivePath.GetCount() >= kHistorySize)
      currentItem = m_ArchivePath.GetCount() - 1;
  }
  else
    m_ArchivePath.GetLBText(currentItem, s);
  s.TrimLeft();
  s.TrimRight();
  m_RegistryInfo.HistoryArchives.Clear();
  AddUniqueString(m_RegistryInfo.HistoryArchives, (const TCHAR *)s);
  m_Info.ArchiveName = s;
  m_Info.UpdateMode = NCompressDialog::NUpdateMode::EEnum(m_UpdateMode.GetCurSel());

  m_Info.Method = NCompressDialog::NMethod::EEnum(m_Method.GetCurSel());
  m_Info.ArchiverInfoIndex = m_Format.GetCurSel();

  m_Info.SFXMode = IsButtonCheckedBool(IDC_COMPRESS_SFX);
  m_RegistryInfo.Solid = m_Info.Solid = IsButtonCheckedBool(IDC_COMPRESS_SOLID);
  m_RegistryInfo.MultiThread = m_Info.MultiThread = IsButtonCheckedBool(IDC_COMPRESS_MULTI_THREAD);
  m_RegistryInfo.EncryptHeaders = EncryptHeaders = IsButtonCheckedBool(IDC_COMPRESS_CHECK_ENCRYPT_FILE_NAMES);

  m_Options.GetText(m_Info.Options);
  CSysString volumeString;
  m_Volume.GetText(volumeString);
  volumeString.Trim();
  m_Info.VolumeSizeIsDefined = ParseVolumeSize(
      volumeString, m_Info.VolumeSize);
  /*
  if (!m_Info.VolumeSizeIsDefined && !volumeString.IsEmpty())
    MessageBox(0, TEXT("Incorrect volume size"), TEXT("7-Zip"), 0);
  */

  for(int i = 0; i < m_ArchivePath.GetCount(); i++)
    if(i != currentItem)
    {
      m_ArchivePath.GetLBText(i, s);
      s.Trim();
      AddUniqueString(m_RegistryInfo.HistoryArchives, (const TCHAR *)s);
    }
  
  ////////////////////
  // Method

  m_RegistryInfo.SetMethod(m_Method.GetCurSel());
  m_RegistryInfo.SetLastArchiveType(m_ArchiverInfoList[
      m_Info.ArchiverInfoIndex].Name);

  m_RegistryInfo.ShowPassword = (IsButtonChecked(
      IDC_COMPRESS_CHECK_SHOW_PASSWORD) == BST_CHECKED);

  SaveCompressionInfo(m_RegistryInfo);
  
  CModalDialog::OnOK();
}

static LPCTSTR kHelpTopic = TEXT("fm/plugins/7-zip/add.htm");

void CCompressDialog::OnHelp() 
{
  ShowHelpWindow(NULL, kHelpTopic);
}

bool CCompressDialog::OnCommand(int code, int itemID, LPARAM lParam)
{
  if (code == CBN_SELCHANGE && itemID == IDC_COMPRESS_COMBO_FORMAT)
  {
    OnSelChangeComboFormat();
    CheckControlsEnable();
    CWindow sfxButton = GetItem(IDC_COMPRESS_SFX);
    bool sfxMode = sfxButton.IsEnabled() && IsButtonCheckedBool(IDC_COMPRESS_SFX);
    if (sfxMode)
      OnButtonSFX();
    return true;
  }
  return CModalDialog::OnCommand(code, itemID, lParam);
}

void CCompressDialog::OnSelChangeComboFormat() 
{
  SaveOptions();
  CSysString fileName;
  m_ArchivePath.GetText(fileName);

  const CArchiverInfo &prevArchiverInfo = m_ArchiverInfoList[m_PrevFormat];
  if (prevArchiverInfo.KeepName || m_Info.KeepName)
  {
    const CSysString &prevExtension = GetSystemString(prevArchiverInfo.GetMainExtension());
    const int prevExtensionLen = prevExtension.Length();
    if (fileName.Right(prevExtensionLen).CompareNoCase(prevExtension) == 0)
    {
      int pos = fileName.Length() - prevExtensionLen;
      CSysString temp = fileName.Left(pos);
      if (pos > 1)
      {
        int dotPos = fileName.ReverseFind('.');
        if (dotPos == pos - 1)
          fileName = fileName.Left(dotPos);
      }
    }
  }
  SetArchiveName(fileName);
  SetOptions();
}

void CCompressDialog::SetArchiveName(const CSysString &name)
{
  CSysString fileName = name;
  m_Info.ArchiverInfoIndex = m_Format.GetCurSel();
  const CArchiverInfo &archiverInfo = m_ArchiverInfoList[m_Info.ArchiverInfoIndex];
  m_PrevFormat = m_Info.ArchiverInfoIndex;

  if (archiverInfo.KeepName)
  {
    fileName = ArchiveNameSrc;
  }
  else
  {
    if (!m_Info.KeepName)
    {
      int dotPos = fileName.ReverseFind('.');
      int slashPos = MyMax(fileName.ReverseFind('\\'), fileName.ReverseFind('/'));
      if (dotPos > slashPos)
        fileName = fileName.Left(dotPos);
    }
  }
  fileName += '.';
  fileName += GetSystemString(archiverInfo.GetMainExtension());
  m_ArchivePath.SetText(fileName);
}

int CCompressDialog::FindFormat(const UString &name)
{
  for (int i = 0; i < m_RegistryInfo.FormatOptionsVector.Size(); i++)
  {
    const NCompression::CFormatOptions &formatOptions = 
        m_RegistryInfo.FormatOptionsVector[i];
    if (GetUnicodeString(formatOptions.FormatID) == name)
      return i;
  }
  return -1;
}


void CCompressDialog::SaveOptions()
{
  const CArchiverInfo &archiverInfo = m_ArchiverInfoList[m_Info.ArchiverInfoIndex];
  int index = FindFormat(archiverInfo.Name);
  m_Options.GetText(m_Info.Options);
  if (index >= 0)
  {
    NCompression::CFormatOptions &formatOptions = 
        m_RegistryInfo.FormatOptionsVector[index];
    formatOptions.Options = m_Info.Options;
  }
  else
  {
    NCompression::CFormatOptions formatOptions;
    formatOptions.FormatID = GetSystemString(archiverInfo.Name);
    formatOptions.Options = m_Info.Options;
    m_RegistryInfo.FormatOptionsVector.Add(formatOptions);
  }
}

void CCompressDialog::SetOptions()
{
  const CArchiverInfo &archiverInfo = m_ArchiverInfoList[m_Format.GetCurSel()];
  m_Options.SetText(TEXT(""));
  int index = FindFormat(archiverInfo.Name);
  if (index >= 0)
  {
    const NCompression::CFormatOptions &formatOptions = 
        m_RegistryInfo.FormatOptionsVector[index];
    m_Options.SetText(formatOptions.Options);
  }
}