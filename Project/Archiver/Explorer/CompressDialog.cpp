// CompressDialog.cpp

#include "StdAfx.h"

#include "resource.h"
#include "CompressDialog.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/ResourceString.h"

#include "../../FileManager/HelpUtils.h"

#include "Common/Defs.h"

#ifdef LANG        
#include "../../FileManager/LangUtils.h"
#endif

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDC_STATIC_COMPRESS_ARCHIVE, 0x02000D01 },
  { IDC_STATIC_COMPRESS_FORMAT, 0x02000D03 },
  { IDC_STATIC_COMPRESS_METHOD, 0x02000D04 },
  { IDC_COMPRESS_SOLID, 0x02000D05 },
  { IDC_STATIC_COMPRESS_PARAMETERS, 0x02000D06 },
  { IDC_STATIC_COMPRESS_UPDATE_MODE, 0x02000D02 },
  { IDC_STATIC_COMPRESS_OPTIONS, 0x02000D07 },
  { IDC_COMPRESS_SFX, 0x02000D08 },
  { IDC_COMPRESS_PASSWORD, 0x02000802 },
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
  void Add(LPCTSTR aString);
  void SetForBuffer(LPTSTR aBuffer);
};

const TCHAR kDelimiterSymbol = _T(' ');
void CDoubleZeroStringList::Add(LPCTSTR aString)
{
  m_String += aString;
  m_Indexes.Add(m_String.Length());
  m_String += kDelimiterSymbol;
}

void CDoubleZeroStringList::SetForBuffer(LPTSTR aBuffer)
{
  _tcscpy(aBuffer, m_String);
  for (int i = 0; i < m_Indexes.Size(); i++)
    aBuffer[m_Indexes[i]] = _T('\0');
}


bool CCompressDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000D00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _passwordControl.Init(*this, IDC_COMPRESS_EDIT_PASSWORD);
  _passwordControl.SetText(_T(""));

  m_ArchivePath.Attach(GetItem(IDC_COMPRESS_COMBO_ARCHIVE));
	m_Format.Attach(GetItem(IDC_COMPRESS_COMBO_FORMAT));
	m_Method.Attach(GetItem(IDC_COMPRESS_COMBO_METHOD));
	m_UpdateMode.Attach(GetItem(IDC_COMPRESS_COMBO_UPDATE_MODE));

  m_Options.Attach(GetItem(IDC_COMPRESS_EDIT_PARAMETERS));

  NZipRegistryManager::ReadCompressionInfo(m_RegistryInfo);


  m_Info.ArchiverInfoIndex = 0;
  for(int i = 0; i < m_ArchiverInfoList.Size(); i++)
  {
    const NZipRootRegistry::CArchiverInfo &archiverInfo = m_ArchiverInfoList[i];
    m_Format.AddString(archiverInfo.Name);
    if (archiverInfo.ClassID == m_RegistryInfo.LastClassID && 
        m_RegistryInfo.LastClassIDDefined)
      m_Info.ArchiverInfoIndex = i;
  }
  m_Format.SetCurSel(m_Info.ArchiverInfoIndex);

  m_Info.ArchiveNameSrc = m_Info.ArchiveName;
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

  m_Method.AddString(
      #ifdef LANG
      LangLoadString(IDS_METHOD_STORE, 0x02000D81)
      #else
      MyLoadString(IDS_METHOD_STORE)
      #endif
      );
  m_Method.AddString(
      #ifdef LANG
      LangLoadString(IDS_METHOD_NORMAL, 0x02000D82)
      #else
      MyLoadString(IDS_METHOD_NORMAL)
      #endif
      );
  m_Method.AddString(
      #ifdef LANG
      LangLoadString(IDS_METHOD_MAXIMUM, 0x02000D83)
      #else
      MyLoadString(IDS_METHOD_MAXIMUM)
      #endif
      );
  
  int aMethodIndex = 1;
  if (m_RegistryInfo.MethodDefined && m_RegistryInfo.Method < 3)
    aMethodIndex = m_RegistryInfo.Method;
  m_Method.SetCurSel(aMethodIndex);

  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_ADD, 0x02000DA1));
  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_UPDATE, 0x02000DA2));
  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_FRESH, 0x02000DA3));
  m_UpdateMode.AddString(LangLoadString(IDS_COMPRESS_UPDATE_MODE_SYNCHRONIZE, 0x02000DA4));

  m_UpdateMode.SetCurSel(0);

  m_Info.SolidMode = m_RegistryInfo.SolidMode;


  CheckButton(IDC_COMPRESS_SOLID, m_Info.SolidMode);
  CheckButton(IDC_COMPRESS_SFX, m_Info.SFXMode);
  
  CheckSolidEnable();
  CheckSFXEnable();
  CheckPasswordEnable();

  OnButtonSFX();



  return CModalDialog::OnInit();
}

namespace NCompressDialog
{
  bool CInfo::GetFullPathName(CSysString &aResult) const
  {
    ::SetCurrentDirectory(CurrentDirPrefix);
    return MyGetFullPathName(ArchiveName, aResult);
  }
}

bool CCompressDialog::OnButtonClicked(int aButtonID, HWND aButtonHWND)
{
  switch(aButtonID)
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
  }
  return CModalDialog::OnButtonClicked(aButtonID, aButtonHWND);
}

void CCompressDialog::CheckSolidEnable()
{
  int formatIndex = m_Format.GetCurSel();
  const NZipRootRegistry::CArchiverInfo &archiverInfo = 
      m_ArchiverInfoList[formatIndex];
  bool enable = (archiverInfo.Name == TEXT("7z"));
  m_Info.SolidModeIsAllowed = enable;
  CWindow aSFXButton = GetItem(IDC_COMPRESS_SOLID);
  aSFXButton.Enable(enable);
}

void CCompressDialog::CheckSFXEnable()
{
  int formatIndex = m_Format.GetCurSel();
  const NZipRootRegistry::CArchiverInfo &archiverInfo = 
      m_ArchiverInfoList[formatIndex];
  bool enable = (archiverInfo.Name == TEXT("7z"));

  CWindow aSFXButton = GetItem(IDC_COMPRESS_SFX);
  aSFXButton.Enable(enable);
}

void CCompressDialog::CheckPasswordEnable()
{
  int formatIndex = m_Format.GetCurSel();
  const NZipRootRegistry::CArchiverInfo &archiverInfo = 
      m_ArchiverInfoList[formatIndex];
  bool enable = (archiverInfo.Name.CompareNoCase(TEXT("zip")) == 0) || 
      (archiverInfo.Name.CompareNoCase(TEXT("7z")) == 0);
  CWindow window = GetItem(IDC_COMPRESS_PASSWORD);
  window.Enable(enable);
  window = GetItem(IDC_COMPRESS_EDIT_PASSWORD);
  window.Enable(enable);
}


void CCompressDialog::OnButtonSFX()
{
  CWindow aSFXButton = GetItem(IDC_COMPRESS_SFX);
  bool aSFXMode = aSFXButton.IsEnabled() && IsButtonCheckedBool(IDC_COMPRESS_SFX);
  if (aSFXMode)
  {
    CSysString aFileName;
    m_ArchivePath.GetText(aFileName);
    int aDotPos = aFileName.ReverseFind('.');
    int aSlashPos = aFileName.ReverseFind('\\');
    if (aDotPos >= 0 && aDotPos > aSlashPos)
      aFileName = aFileName.Left(aDotPos);
    aFileName += TEXT(".exe");
    m_ArchivePath.SetText(aFileName);
  }
  else
    OnSelChangeComboFormat();
}

void CCompressDialog::OnButtonSetArchive() 
{
  const kBufferSize = MAX_PATH * 2;
  TCHAR aBuffer[kBufferSize];
  CSysString aFileName;
  m_ArchivePath.GetText(aFileName);
  aFileName.TrimLeft();
  aFileName.TrimRight();
  m_Info.ArchiveName = aFileName;
  CSysString aFullFileName;
  if (!m_Info.GetFullPathName(aFullFileName))
  {
    aFullFileName = m_Info.ArchiveName;
    // AfxMessageBox("Incorrect archive path");;
    return;
  }
  _tcscpy(aBuffer, aFullFileName);

  OPENFILENAME anInfo;
  anInfo.lStructSize = sizeof(anInfo); 
  anInfo.hwndOwner = HWND(*this); 
  anInfo.hInstance = 0; 
  

  const kFilterBufferSize = MAX_PATH;
  TCHAR aFilterBuffer[kFilterBufferSize];
  CDoubleZeroStringList aDoubleZeroStringList;
  // aDoubleZeroStringList.Add(_T("Zip Files (*.zip)"));
  // aDoubleZeroStringList.Add(_T("*.zip"));
  CSysString aString = LangLoadString(IDS_OPEN_TYPE_ALL_FILES, 0x02000DB1);
  aString += _T(" (*.*)");
  aDoubleZeroStringList.Add(aString);
  aDoubleZeroStringList.Add(_T("*.*"));
  aDoubleZeroStringList.SetForBuffer(aFilterBuffer);
  anInfo.lpstrFilter = aFilterBuffer; 
  
  
  anInfo.lpstrCustomFilter = NULL; 
  anInfo.nMaxCustFilter = 0; 
  anInfo.nFilterIndex = 0; 
  
  anInfo.lpstrFile = aBuffer; 
  anInfo.nMaxFile = kBufferSize;
  
  anInfo.lpstrFileTitle = NULL; 
    anInfo.nMaxFileTitle = 0; 
  
  anInfo.lpstrInitialDir= NULL; 

  CSysString aTitle = LangLoadString(IDS_COMPRESS_SET_ARCHIVE_DIALOG_TITLE, 0x02000D90);
  
  anInfo.lpstrTitle = aTitle;

  anInfo.Flags = OFN_EXPLORER | OFN_HIDEREADONLY; 
  anInfo.nFileOffset = 0; 
  anInfo.nFileExtension = 0; 
  anInfo.lpstrDefExt = NULL; 
  
  anInfo.lCustData = 0; 
  anInfo.lpfnHook = NULL; 
  anInfo.lpTemplateName = NULL; 

  if(!GetOpenFileName(&anInfo))
    return;
  m_ArchivePath.SetText(aBuffer);
}


// in ExtractDialog.cpp
extern void AddUniqueString(CSysStringVector &aList, const CSysString &aString);

void CCompressDialog::OnOK() 
{
  _passwordControl.GetText(Password);
  SaveOptions();
  int aCurrentItem = m_ArchivePath.GetCurSel();
  CSysString aString;
  if(aCurrentItem == CB_ERR)
  {
    m_ArchivePath.GetText(aString);
    if(m_ArchivePath.GetCount() >= kHistorySize)
      aCurrentItem = m_ArchivePath.GetCount() - 1;
  }
  else
    m_ArchivePath.GetLBText(aCurrentItem, aString);
  aString.TrimLeft();
  aString.TrimRight();
  m_RegistryInfo.HistoryArchives.Clear();
  AddUniqueString(m_RegistryInfo.HistoryArchives, (const TCHAR *)aString);
  m_Info.ArchiveName = aString;
  m_Info.UpdateMode = NCompressDialog::NUpdateMode::EEnum(m_UpdateMode.GetCurSel());

  m_Info.Method = NCompressDialog::NMethod::EEnum(m_Method.GetCurSel());
  m_Info.ArchiverInfoIndex = m_Format.GetCurSel();

  m_Info.SFXMode = IsButtonCheckedBool(IDC_COMPRESS_SFX);
  m_RegistryInfo.SolidMode = m_Info.SolidMode = IsButtonCheckedBool(IDC_COMPRESS_SOLID);

  m_Options.GetText(m_Info.Options);

  for(int i = 0; i < m_ArchivePath.GetCount(); i++)
    if(i != aCurrentItem)
    {
      m_ArchivePath.GetLBText(i, aString);
      aString.TrimLeft();
      aString.TrimRight();
      AddUniqueString(m_RegistryInfo.HistoryArchives, (const TCHAR *)aString);
    }
  
  ////////////////////
  // Method

  m_RegistryInfo.SetMethod(m_Method.GetCurSel());
  m_RegistryInfo.SetLastClassID(m_ArchiverInfoList[
      m_Info.ArchiverInfoIndex].ClassID);

  NZipRegistryManager::SaveCompressionInfo(m_RegistryInfo);
  
  CModalDialog::OnOK();
}

static LPCTSTR kHelpTopic = _T("fm/plugins/7-zip/add.htm");

void CCompressDialog::OnHelp() 
{
  ShowHelpWindow(NULL, kHelpTopic);
}

bool CCompressDialog::OnCommand(int aCode, int anItemID, LPARAM lParam)
{
  if (aCode == CBN_SELCHANGE && anItemID == IDC_COMPRESS_COMBO_FORMAT)
  {
    OnSelChangeComboFormat();
    CheckSolidEnable();
    CheckSFXEnable();
    CheckPasswordEnable();
    CWindow aSFXButton = GetItem(IDC_COMPRESS_SFX);
    bool aSFXMode = aSFXButton.IsEnabled() && IsButtonCheckedBool(IDC_COMPRESS_SFX);
    if (aSFXMode)
      OnButtonSFX();
    return true;
  }
  return CModalDialog::OnCommand(aCode, anItemID, lParam);
}

void CCompressDialog::OnSelChangeComboFormat() 
{
  SaveOptions();
  CSysString aFileName;
  m_ArchivePath.GetText(aFileName);

  const NZipRootRegistry::CArchiverInfo &aPrevArchiverInfo = m_ArchiverInfoList[m_PrevFormat];
  if (aPrevArchiverInfo.KeepName)
  {
    const CSysString &aPrevExtension = aPrevArchiverInfo.Extension;
    const int aPrevExtensionLen = aPrevExtension.Length();
    if (aFileName.Right(aPrevExtensionLen).CompareNoCase(aPrevExtension) == 0)
    {
      int aPos = aFileName.Length() - aPrevExtensionLen;
      CSysString aTemp = aFileName.Left(aPos);
      if (aPos > 1)
      {
        int aDotPos = aFileName.ReverseFind('.');
        if (aDotPos == aPos - 1)
          aFileName = aFileName.Left(aDotPos);
      }
    }
  }
  SetArchiveName(aFileName);
  SetOptions();
}

void CCompressDialog::SetArchiveName(const CSysString &aName)
{
  CSysString aFileName = aName;
  m_Info.ArchiverInfoIndex = m_Format.GetCurSel();
  const NZipRootRegistry::CArchiverInfo &archiverInfo = 
      m_ArchiverInfoList[m_Info.ArchiverInfoIndex];
  m_PrevFormat = m_Info.ArchiverInfoIndex;

  if (archiverInfo.KeepName)
    aFileName = m_Info.ArchiveNameSrc;
  else
  {
    int aDotPos = aFileName.ReverseFind('.');
    int aSlashPos = MyMax(aFileName.ReverseFind('\\'), aFileName.ReverseFind('/'));
    if (aDotPos > aSlashPos)
      aFileName = aFileName.Left(aDotPos);
  }
  aFileName += '.';
  aFileName += archiverInfo.Extension;
  m_ArchivePath.SetText(aFileName);
}

int CCompressDialog::FindFormat(const CSysString &aName)
{
  for (int i = 0; i < m_RegistryInfo.FormatOptionsVector.Size(); i++)
  {
    const NZipSettings::NCompression::CFormatOptions &aFormatOptions = 
        m_RegistryInfo.FormatOptionsVector[i];
    if (aFormatOptions.FormatID == aName)
      return i;
  }
  return -1;
}


void CCompressDialog::SaveOptions()
{
  const NZipRootRegistry::CArchiverInfo &archiverInfo = 
      m_ArchiverInfoList[m_Info.ArchiverInfoIndex];
  int anIndex = FindFormat(archiverInfo.Name);
  m_Options.GetText(m_Info.Options);
  if (anIndex >= 0)
  {
    NZipSettings::NCompression::CFormatOptions &aFormatOptions = 
        m_RegistryInfo.FormatOptionsVector[anIndex];
    aFormatOptions.Options = m_Info.Options;
  }
  else
  {
    NZipSettings::NCompression::CFormatOptions aFormatOptions;
    aFormatOptions.FormatID = archiverInfo.Name;
    aFormatOptions.Options = m_Info.Options;
    m_RegistryInfo.FormatOptionsVector.Add(aFormatOptions);
  }
}

void CCompressDialog::SetOptions()
{
  const NZipRootRegistry::CArchiverInfo &archiverInfo = 
      m_ArchiverInfoList[m_Format.GetCurSel()];
  m_Options.SetText(TEXT(""));
  int anIndex = FindFormat(archiverInfo.Name);
  if (anIndex >= 0)
  {
    const NZipSettings::NCompression::CFormatOptions &aFormatOptions = 
        m_RegistryInfo.FormatOptionsVector[anIndex];
    m_Options.SetText(aFormatOptions.Options);
  }
}