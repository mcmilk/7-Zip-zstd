// ExtractDialog.cpp

#include "StdAfx.h"

// #include <HtmlHelp.h>

#include "ExtractDialog.h"

#include "Windows/Shell.h"
#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/ResourceString.h"

#ifndef NO_REGISTRY
#include "../Common/HelpUtils.h"
#endif

#include "../Common/ZipSettings.h"

#ifdef LANG        
#include "../Common/LangUtils.h"
#endif

// #include "Help/Context/Extract.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;
using namespace NShell;


using namespace NZipSettings;

static const int kPathnamesButtons[] =
{
  IDC_EXTRACT_RADIO_FULL_PATHNAMES,
  IDC_EXTRACT_RADIO_CURRENT_PATHNAMES,
  IDC_EXTRACT_RADIO_NO_PATHNAMES
};
static const int kNumPathnamesButtons = sizeof(kPathnamesButtons) / sizeof(kPathnamesButtons[0]);

static const int kOverwriteButtons[] =
{
  IDC_EXTRACT_RADIO_ASK_BEFORE_OVERWRITE,
  IDC_EXTRACT_RADIO_OVERWRITE_WITHOUT_PROMPT,
  IDC_EXTRACT_RADIO_SKIP_EXISTING_FILES,
  IDC_EXTRACT_RADIO_AUTO_RENAME
};
static const int kNumOverwriteButtons = sizeof(kOverwriteButtons) / sizeof(kOverwriteButtons[0]);

static const int kFilesButtons[] =
{
  IDC_EXTRACT_RADIO_SELECTED_FILES,
  IDC_EXTRACT_RADIO_ALL_FILES
};
static const int kNumFilesButtons = sizeof(kFilesButtons) / sizeof(kFilesButtons[0]);

#ifndef _SFX
int CExtractDialog::GetPathNameMode() const
{
  for (int i = 0; i < kNumPathnamesButtons; i++)
    if(IsButtonCheckedBool(kPathnamesButtons[i]))
      return i;
  throw 0;
}

int CExtractDialog::GetOverwriteMode() const
{
  for (int i = 0; i < kNumOverwriteButtons; i++)
    if(IsButtonCheckedBool(kOverwriteButtons[i]))
      return i;
  throw 0;
}

int CExtractDialog::GetFilesMode() const
{
  for (int i = 0; i < kNumFilesButtons; i++)
    if(IsButtonCheckedBool(kFilesButtons[i]))
      return i;
  throw 0;
}

#endif

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDC_STATIC_EXTRACT_EXTRACT_TO,      0x02000801 },
  { IDC_EXTRACT_PATH_MODE,               0x02000810 },
  { IDC_EXTRACT_RADIO_FULL_PATHNAMES,    0x02000811 },
  { IDC_EXTRACT_RADIO_CURRENT_PATHNAMES, 0x02000812 },
  { IDC_EXTRACT_RADIO_NO_PATHNAMES,      0x02000813 },
  { IDC_EXTRACT_OVERWRITE_MODE,                 0x02000820 },
  { IDC_EXTRACT_RADIO_ASK_BEFORE_OVERWRITE,     0x02000821 },
  { IDC_EXTRACT_RADIO_OVERWRITE_WITHOUT_PROMPT, 0x02000822 },
  { IDC_EXTRACT_RADIO_SKIP_EXISTING_FILES,      0x02000823 },
  { IDC_EXTRACT_RADIO_AUTO_RENAME,              0x02000824 },
  { IDC_EXTRACT_FILES,                0x02000830 },
  { IDC_EXTRACT_RADIO_SELECTED_FILES, 0x02000831 },
  { IDC_EXTRACT_RADIO_ALL_FILES,      0x02000832 },
  { IDC_EXTRACT_PASSWORD,        0x02000802 },
  { IDOK,     0x02000702 },
  { IDCANCEL, 0x02000710 },
  { IDHELP,   0x02000720 }

};
#endif

bool CExtractDialog::Init(
    #ifndef  NO_REGISTRY
    CZipRegistryManager *aManager, 
    #endif
    const CSysString &aFileName)
{
  #ifndef  NO_REGISTRY
  m_ZipRegistryManager = aManager;
  #endif

  #ifdef _SFX
  NDirectory::GetOnlyDirPrefix(aFileName, m_DirectoryPath);
  #else
  int aFileNamePartStartIndex;
  CSysString aFullPathName;
  NDirectory::MyGetFullPathName(aFileName, aFullPathName, aFileNamePartStartIndex);
  m_DirectoryPath = aFileName.Left(aFileNamePartStartIndex);
  CSysString aName = aFileName.Mid(aFileNamePartStartIndex);
  CSysString aPureName, aDot, anExtension;
  SplitNameToPureNameAndExtension(aName, 
      aPureName, aDot, anExtension);
  if (!aDot.IsEmpty())
    m_DirectoryPath += aPureName;
  #endif
  return true;
}

// static const kWildcardsButtonIndex = 2;

static const kHistorySize = 8;

bool CExtractDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000800);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  #ifndef _SFX
  m_PasswordControl.Init(*this, IDC_EXTRACT_EDIT_PASSWORD);
  m_PasswordControl.SetText(_T(""));
  #endif

  NExtraction::CInfo anExtractionInfo;

  #ifdef NO_REGISTRY
  anExtractionInfo.PathMode = NExtraction::NPathMode::kFullPathnames;
  anExtractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kAskBefore;
  // anExtractionInfo.Paths = NExtraction::NPathMode::kFullPathnames;
  #else
  m_ZipRegistryManager->ReadExtractionInfo(anExtractionInfo);
  #endif

  m_Path.Attach(GetItem(IDC_EXTRACT_COMBO_PATH));
  m_Path.SetText(m_DirectoryPath);
  
  #ifndef NO_REGISTRY
  for(int i = 0; i < anExtractionInfo.Paths.Size() && i < kHistorySize; i++)
    m_Path.AddString(anExtractionInfo.Paths[i]);
  #endif
  /*
  if(anExtractionInfo.Paths.Size() > 0) 
    m_Path.SetCurSel(0);
  else
    m_Path.SetCurSel(-1);
  */

  
  m_PathMode = anExtractionInfo.PathMode;
  m_OverwriteMode = anExtractionInfo.OverwriteMode;
  
  #ifndef _SFX
  CheckRadioButton(kPathnamesButtons[0], kPathnamesButtons[kNumPathnamesButtons - 1], 
      kPathnamesButtons[m_PathMode]);

  CheckRadioButton(kOverwriteButtons[0], kOverwriteButtons[kNumOverwriteButtons - 1], 
      kOverwriteButtons[m_OverwriteMode]);

  CheckRadioButton(kFilesButtons[0], kFilesButtons[kNumFilesButtons - 1], 
      kFilesButtons[m_FilesMode]);

  CWindow aSelectedFilesWindow = GetItem(IDC_EXTRACT_RADIO_SELECTED_FILES);
  aSelectedFilesWindow.Enable(m_EnableSelectedFilesButton);

  #endif

 
  // CWindow aFilesWindow = GetItem(IDC_EXTRACT_RADIO_FILES);
  // aFilesWindow.Enable(m_EnableFilesButton);

  // UpdateWildCardState();
  return CModalDialog::OnInit();
}

bool CExtractDialog::OnButtonClicked(int aButtonID, HWND aButtonHWND)
{
  /*
  for (int i = 0; i < kNumFilesButtons; i++)
    if (aButtonID == kFilesButtons[i])
    {
      UpdateWildCardState();
      return true;
    }
  */
  switch(aButtonID)
  {
    case IDC_EXTRACT_BUTTON_SET_PATH:
      OnButtonSetPath();
      return true;
  }
  return CModalDialog::OnButtonClicked(aButtonID, aButtonHWND);
}

void CExtractDialog::OnButtonSetPath() 
{
  CSysString aCurrentPath;
  m_Path.GetText(aCurrentPath);

  #ifdef LANG        
  CSysString aTitle = LangLoadString(IDS_EXTRACT_SET_FOLDER, 0x02000881);
  #else
  CSysString aTitle = MyLoadString(IDS_EXTRACT_SET_FOLDER);
  #endif


  CSysString aResultPath;
  if (!NShell::BrowseForFolder(HWND(*this), aTitle, aCurrentPath, aResultPath))
    return;
  #ifndef NO_REGISTRY
  m_Path.SetCurSel(-1);
  #endif
  m_Path.SetText(aResultPath);
}

void AddUniqueString(CSysStringVector &aList, const CSysString &aString)
{
  CSysString aStringLoc = aString;
  for(int i = 0; i < aList.Size(); i++)
    if (aStringLoc.CollateNoCase(aList[i]) == 0)
      return;
  aList.Add(aStringLoc);
}

void CExtractDialog::OnOK() 
{
  #ifndef _SFX
  m_PathMode = GetPathNameMode();
  m_OverwriteMode = GetOverwriteMode();
  m_FilesMode = (NExtractionDialog::NFilesMode::EEnum)GetFilesMode();

  m_PasswordControl.GetText(m_Password);
  #endif

  NExtraction::CInfo anExtractionInfo;
  anExtractionInfo.PathMode = NExtraction::NPathMode::EEnum(m_PathMode);
  anExtractionInfo.OverwriteMode = NExtraction::NOverwriteMode::EEnum(m_OverwriteMode);
  
  CSysString aString;
  
  #ifdef NO_REGISTRY
  
  m_Path.GetText(aString);
  
  #else

  int aCurrentItem = m_Path.GetCurSel();
  if(aCurrentItem == CB_ERR)
  {
    m_Path.GetText(aString);
    if(m_Path.GetCount() >= kHistorySize)
      aCurrentItem = m_Path.GetCount() - 1;
  }
  else
    m_Path.GetLBText(aCurrentItem, aString);
  
  #endif

  aString.TrimLeft();
  aString.TrimRight();
  AddUniqueString(anExtractionInfo.Paths, (const TCHAR *)aString);
  m_DirectoryPath = aString;
  #ifndef  NO_REGISTRY
  for(int i = 0; i < m_Path.GetCount(); i++)
    if(i != aCurrentItem)
    {
      m_Path.GetLBText(i, aString);
      aString.TrimLeft();
      aString.TrimRight();
      AddUniqueString(anExtractionInfo.Paths, (const TCHAR *)aString);
    }
  m_ZipRegistryManager->SaveExtractionInfo(anExtractionInfo);
  #endif
  CModalDialog::OnOK();
}

/*
void CExtractDialog::UpdateWildCardState()
{
  // UpdateData(TRUE);
  // m_Wildcards.EnableWindow(BoolToBOOL(m_Files == kWildcardsButtonIndex));
}
*/

/*
static DWORD aHelpArray[] = 
{
  IDC_EXTRACT_COMBO_PATH, IDH_EXTRACT_COMBO_PATH,
  IDC_EXTRACT_BUTTON_SET_PATH, IDH_EXTRACT_BUTTON_SET_PATH,

  IDC_EXTRACT_PATH_MODE, IDH_EXTRACT_PATH_MODE,
  IDC_EXTRACT_RADIO_FULL_PATHNAMES, IDH_EXTRACT_RADIO_FULL_PATHNAMES,
  IDC_EXTRACT_RADIO_CURRENT_PATHNAMES,IDH_EXTRACT_RADIO_CURRENT_PATHNAMES,
  IDC_EXTRACT_RADIO_NO_PATHNAMES,IDH_EXTRACT_RADIO_NO_PATHNAMES,

  IDC_EXTRACT_OVERWRITE_MODE, IDH_EXTRACT_OVERWRITE_MODE,
  IDC_EXTRACT_RADIO_ASK_BEFORE_OVERWRITE, IDH_EXTRACT_RADIO_ASK_BEFORE_OVERWRITE,
  IDC_EXTRACT_RADIO_OVERWRITE_WITHOUT_PROMPT, IDH_EXTRACT_RADIO_OVERWRITE_WITHOUT_PROMPT,
  IDC_EXTRACT_RADIO_SKIP_EXISTING_FILES, IDH_EXTRACT_RADIO_SKIP_EXISTING_FILES,

  IDC_EXTRACT_FILES, IDH_EXTRACT_FILES,
  IDC_EXTRACT_RADIO_SELECTED_FILES, IDH_EXTRACT_RADIO_SELECTED_FILES,
  IDC_EXTRACT_RADIO_ALL_FILES, IDH_EXTRACT_RADIO_ALL_FILES,
  IDC_EXTRACT_RADIO_FILES, IDH_EXTRACT_RADIO_FILES,
  IDC_EXTRACT_EDIT_WILDCARDS, IDH_EXTRACT_EDIT_WILDCARDS,
  0,0
};
*/


void CExtractDialog::GetModeInfo(NExtractionDialog::CModeInfo &aModeInfo)
{
  aModeInfo.OverwriteMode = NExtractionDialog::NOverwriteMode::EEnum(m_OverwriteMode);
  aModeInfo.PathMode = NExtractionDialog::NPathMode::EEnum(m_PathMode);
  aModeInfo.FilesMode = NExtractionDialog::NFilesMode::EEnum(m_FilesMode);
  aModeInfo.FileList.Clear();
}
  
#ifndef  NO_REGISTRY
static LPCTSTR kHelpTopic = _T("gui/extract.htm");
void CExtractDialog::OnHelp() 
{
  ShowHelpWindow(NULL, kHelpTopic);
  CModalDialog::OnHelp();
  /*
  if (pHelpInfo->iContextType == HELPINFO_WINDOW)
  {
    return ::HtmlHelp((HWND)pHelpInfo->hItemHandle, 
        _T("C:\\SRC\\VC\\ZipView\\Help\\7zip.chm::/Context/Extract.txt"), 
          HH_TP_HELP_WM_HELP, (DWORD)(LPVOID)aHelpArray) != NULL;
  }
  */
}
#endif

