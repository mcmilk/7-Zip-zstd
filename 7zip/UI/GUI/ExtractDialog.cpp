// ExtractDialog.cpp

#include "StdAfx.h"

// #include <HtmlHelp.h>

#include "ExtractDialog.h"

#include "Windows/Shell.h"
#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/ResourceString.h"

#ifndef NO_REGISTRY
#include "../../FileManager/HelpUtils.h"
#endif

#include "../Common/ZipRegistry.h"

#ifdef LANG        
#include "../../FileManager/LangUtils.h"
#endif

#include "../Resource/Extract/resource.h"
#include "../Resource/ExtractDialog/resource.h"

// #include "Help/Context/Extract.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

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
  { IDC_EXTRACT_CHECK_SHOW_PASSWORD, 0x02000B02 },
  { IDOK,     0x02000702 },
  { IDCANCEL, 0x02000710 },
  { IDHELP,   0x02000720 }

};
#endif

bool CExtractDialog::Init(const CSysString &aFileName)
{
  #ifdef _SFX
  NDirectory::GetOnlyDirPrefix(aFileName, _directoryPath);
  #else
  int fileNamePartStartIndex;
  CSysString fullPathName;
  NDirectory::MyGetFullPathName(aFileName, fullPathName, fileNamePartStartIndex);
  _directoryPath = fullPathName.Left(fileNamePartStartIndex);
  CSysString name = fullPathName.Mid(fileNamePartStartIndex);
  CSysString pureName, dot, extension;
  SplitNameToPureNameAndExtension(name, pureName, dot, extension);
  if (!dot.IsEmpty())
    _directoryPath += pureName;
  #endif
  return true;
}

// static const int kWildcardsButtonIndex = 2;

static const int kHistorySize = 8;

bool CExtractDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000800);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  #ifndef _SFX
  _passwordControl.Attach(GetItem(IDC_EXTRACT_EDIT_PASSWORD));
  _passwordControl.SetText(_password);
  _passwordControl.SetPasswordChar(TEXT('*'));
  #endif

  NExtraction::CInfo extractionInfo;

  #ifdef NO_REGISTRY
  extractionInfo.PathMode = NExtraction::NPathMode::kFullPathnames;
  extractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kAskBefore;
  // extractionInfo.Paths = NExtraction::NPathMode::kFullPathnames;
  #else
  ReadExtractionInfo(extractionInfo);
  CheckButton(IDC_EXTRACT_CHECK_SHOW_PASSWORD, extractionInfo.ShowPassword);
  UpdatePasswordControl();
  #endif


  _path.Attach(GetItem(IDC_EXTRACT_COMBO_PATH));
  _path.SetText(_directoryPath);
  
  #ifndef NO_REGISTRY
  for(int i = 0; i < extractionInfo.Paths.Size() && i < kHistorySize; i++)
    _path.AddString(extractionInfo.Paths[i]);
  #endif
  /*
  if(extractionInfo.Paths.Size() > 0) 
    _path.SetCurSel(0);
  else
    _path.SetCurSel(-1);
  */

  
  _pathMode = extractionInfo.PathMode;
  _overwriteMode = extractionInfo.OverwriteMode;
  
  #ifndef _SFX
  CheckRadioButton(kPathnamesButtons[0], kPathnamesButtons[kNumPathnamesButtons - 1], 
      kPathnamesButtons[_pathMode]);

  CheckRadioButton(kOverwriteButtons[0], kOverwriteButtons[kNumOverwriteButtons - 1], 
      kOverwriteButtons[_overwriteMode]);

  CheckRadioButton(kFilesButtons[0], kFilesButtons[kNumFilesButtons - 1], 
      kFilesButtons[_filesMode]);

  CWindow aSelectedFilesWindow = GetItem(IDC_EXTRACT_RADIO_SELECTED_FILES);
  aSelectedFilesWindow.Enable(_enableSelectedFilesButton);

  #endif

 
  // CWindow aFilesWindow = GetItem(IDC_EXTRACT_RADIO_FILES);
  // aFilesWindow.Enable(_enableFilesButton);

  // UpdateWildCardState();
  return CModalDialog::OnInit();
}

#ifndef _SFX
void CExtractDialog::UpdatePasswordControl()
{
  _passwordControl.SetPasswordChar((IsButtonChecked(
    IDC_EXTRACT_CHECK_SHOW_PASSWORD) == BST_CHECKED) ? 0: TEXT('*'));
  CSysString password;
  _passwordControl.GetText(password);
  _passwordControl.SetText(password);
}
#endif

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
    #ifndef _SFX
    case IDC_EXTRACT_CHECK_SHOW_PASSWORD:
    {
      UpdatePasswordControl();
      return true;
    }
    #endif
  }
  return CModalDialog::OnButtonClicked(aButtonID, aButtonHWND);
}

void CExtractDialog::OnButtonSetPath() 
{
  CSysString currentPath;
  _path.GetText(currentPath);

  #ifdef LANG        
  CSysString title = LangLoadString(IDS_EXTRACT_SET_FOLDER, 0x02000881);
  #else
  CSysString title = MyLoadString(IDS_EXTRACT_SET_FOLDER);
  #endif


  CSysString aResultPath;
  if (!NShell::BrowseForFolder(HWND(*this), title, currentPath, aResultPath))
    return;
  #ifndef NO_REGISTRY
  _path.SetCurSel(-1);
  #endif
  _path.SetText(aResultPath);
}

void AddUniqueString(CSysStringVector &list, const CSysString &string)
{
  CSysString stringLoc = string;
  for(int i = 0; i < list.Size(); i++)
    if (stringLoc.CollateNoCase(list[i]) == 0)
      return;
  list.Add(stringLoc);
}

void CExtractDialog::OnOK() 
{
  #ifndef _SFX
  _pathMode = GetPathNameMode();
  _overwriteMode = GetOverwriteMode();
  _filesMode = (NExtractionDialog::NFilesMode::EEnum)GetFilesMode();

  _passwordControl.GetText(_password);
  #endif

  NExtraction::CInfo extractionInfo;
  extractionInfo.PathMode = NExtraction::NPathMode::EEnum(_pathMode);
  extractionInfo.OverwriteMode = NExtraction::NOverwriteMode::EEnum(_overwriteMode);
  extractionInfo.ShowPassword = (IsButtonChecked(
          IDC_EXTRACT_CHECK_SHOW_PASSWORD) == BST_CHECKED);
  
  CSysString string;
  
  #ifdef NO_REGISTRY
  
  _path.GetText(string);
  
  #else

  int currentItem = _path.GetCurSel();
  if(currentItem == CB_ERR)
  {
    _path.GetText(string);
    if(_path.GetCount() >= kHistorySize)
      currentItem = _path.GetCount() - 1;
  }
  else
    _path.GetLBText(currentItem, string);
  
  #endif

  string.TrimLeft();
  string.TrimRight();
  AddUniqueString(extractionInfo.Paths, (const TCHAR *)string);
  _directoryPath = string;
  #ifndef  NO_REGISTRY
  for(int i = 0; i < _path.GetCount(); i++)
    if(i != currentItem)
    {
      _path.GetLBText(i, string);
      string.TrimLeft();
      string.TrimRight();
      AddUniqueString(extractionInfo.Paths, (const TCHAR *)string);
    }
  SaveExtractionInfo(extractionInfo);
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
  aModeInfo.OverwriteMode = NExtractionDialog::NOverwriteMode::EEnum(_overwriteMode);
  aModeInfo.PathMode = NExtractionDialog::NPathMode::EEnum(_pathMode);
  aModeInfo.FilesMode = NExtractionDialog::NFilesMode::EEnum(_filesMode);
  aModeInfo.FileList.Clear();
}
  
#ifndef  NO_REGISTRY
static LPCTSTR kHelpTopic = TEXT("fm/plugins/7-zip/extract.htm");
void CExtractDialog::OnHelp() 
{
  ShowHelpWindow(NULL, kHelpTopic);
  CModalDialog::OnHelp();
  /*
  if (pHelpInfo->iContextType == HELPINFO_WINDOW)
  {
    return ::HtmlHelp((HWND)pHelpInfo->hItemHandle, 
        TEXT("C:\\SRC\\VC\\ZipView\\Help\\7zip.chm::/Context/Extract.txt"), 
          HH_TP_HELP_WM_HELP, (DWORD)(LPVOID)aHelpArray) != NULL;
  }
  */
}
#endif

