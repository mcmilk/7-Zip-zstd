// CompressDialog.cpp

#include "StdAfx.h"

#include "resource.h"
#include "CompressDialog.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/ResourceString.h"

#include "../Common/HelpUtils.h"

#include "Common/Defs.h"

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
  m_ArchivePath.Attach(GetItem(IDC_COMPRESS_COMBO_ARCHIVE));
	m_Format.Attach(GetItem(IDC_COMPRESS_COMBO_FORMAT));
	m_Method.Attach(GetItem(IDC_COMPRESS_COMBO_METHOD));
	m_UpdateMode.Attach(GetItem(IDC_COMPRESS_COMBO_UPDATE_MODE));

  m_Options.Attach(GetItem(IDC_COMPRESS_EDIT_OPTIONS));

  m_ZipRegistryManager->ReadCompressionInfo(m_RegistryInfo);


  m_Info.ArchiverInfoIndex = 0;
  for(int i = 0; i < m_ArchiverInfoList.Size(); i++)
  {
    const NZipRootRegistry::CArchiverInfo &anArchiverInfo = m_ArchiverInfoList[i];
    m_Format.AddString(anArchiverInfo.Name);
    if (anArchiverInfo.ClassID == m_RegistryInfo.LastClassID && 
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

  m_Method.AddString(MyLoadString(IDS_METHOD_STORE));
  m_Method.AddString(MyLoadString(IDS_METHOD_NORMAL));
  m_Method.AddString(MyLoadString(IDS_METHOD_MAXIMUM));
  
  int aMethodIndex = 1;
  if (m_RegistryInfo.MethodDefined && m_RegistryInfo.Method < 3)
    aMethodIndex = m_RegistryInfo.Method;
  m_Method.SetCurSel(aMethodIndex);

  m_UpdateMode.AddString(MyLoadString(IDS_COMPRESS_UPDATE_MODE_ADD));
  m_UpdateMode.AddString(MyLoadString(IDS_COMPRESS_UPDATE_MODE_UPDATE));
  m_UpdateMode.AddString(MyLoadString(IDS_COMPRESS_UPDATE_MODE_FRESH));
  m_UpdateMode.AddString(MyLoadString(IDS_COMPRESS_UPDATE_MODE_SYNCHRONIZE));

  m_UpdateMode.SetCurSel(0);

  m_Info.SolidMode = m_RegistryInfo.SolidMode;


  CheckButton(IDC_COMPRESS_SOLID, m_Info.SolidMode);
  CheckButton(IDC_COMPRESS_SFX, m_Info.SFXMode);
  
  CheckSolidEnable();
  CheckSFXEnable();

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
  int aFormatIndex = m_Format.GetCurSel();
  const NZipRootRegistry::CArchiverInfo &anArchiverInfo = 
      m_ArchiverInfoList[aFormatIndex];
  bool anEnable = (anArchiverInfo.Name == "7z");
  m_Info.SolidModeIsAllowed = anEnable;
  CWindow aSFXButton = GetItem(IDC_COMPRESS_SOLID);
  aSFXButton.Enable(anEnable);
}

void CCompressDialog::CheckSFXEnable()
{
  int aFormatIndex = m_Format.GetCurSel();
  const NZipRootRegistry::CArchiverInfo &anArchiverInfo = 
      m_ArchiverInfoList[aFormatIndex];
  bool anEnable = (anArchiverInfo.Name == "7z");

  CWindow aSFXButton = GetItem(IDC_COMPRESS_SFX);
  aSFXButton.Enable(anEnable);
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
  aDoubleZeroStringList.Add(_T("Zip Files (*.zip)"));
  aDoubleZeroStringList.Add(_T("*.zip"));
  CSysString aString = MyLoadString(IDS_OPEN_TYPE_ALL_FILES);
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

  CSysString aTitle = MyLoadString(IDS_COMPRESS_SET_ARCHIVE_DIALOG_TITLE);
  
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

  m_ZipRegistryManager->SaveCompressionInfo(m_RegistryInfo);
  
  CModalDialog::OnOK();
}

static LPCTSTR kHelpTopic = _T("gui/add.htm");

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
  const NZipRootRegistry::CArchiverInfo &anArchiverInfo = 
      m_ArchiverInfoList[m_Info.ArchiverInfoIndex];
  m_PrevFormat = m_Info.ArchiverInfoIndex;

  if (anArchiverInfo.KeepName)
    aFileName = m_Info.ArchiveNameSrc;
  else
  {
    int aDotPos = aFileName.ReverseFind('.');
    int aSlashPos = MyMax(aFileName.ReverseFind('\\'), aFileName.ReverseFind('/'));
    if (aDotPos > aSlashPos)
      aFileName = aFileName.Left(aDotPos);
  }
  aFileName += '.';
  aFileName += anArchiverInfo.Extension;
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
  const NZipRootRegistry::CArchiverInfo &anArchiverInfo = 
      m_ArchiverInfoList[m_Info.ArchiverInfoIndex];
  int anIndex = FindFormat(anArchiverInfo.Name);
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
    aFormatOptions.FormatID = anArchiverInfo.Name;
    aFormatOptions.Options = m_Info.Options;
    m_RegistryInfo.FormatOptionsVector.Add(aFormatOptions);
  }
}

void CCompressDialog::SetOptions()
{
  const NZipRootRegistry::CArchiverInfo &anArchiverInfo = 
      m_ArchiverInfoList[m_Format.GetCurSel()];
  m_Options.SetText(TEXT(""));
  int anIndex = FindFormat(anArchiverInfo.Name);
  if (anIndex >= 0)
  {
    const NZipSettings::NCompression::CFormatOptions &aFormatOptions = 
        m_RegistryInfo.FormatOptionsVector[anIndex];
    m_Options.SetText(aFormatOptions.Options);
  }
}