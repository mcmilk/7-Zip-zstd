// ExtractDialog.cpp

#include "StdAfx.h"

// #include <HtmlHelp.h>

#include "ExtractDialog.h"

#include "Common/StringConvert.h"
#include "Windows/Shell.h"
#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#include "Windows/ResourceString.h"

#ifndef NO_REGISTRY
#include "../FileManager/HelpUtils.h"
#endif

#include "../Common/ZipRegistry.h"

#include "../FileManager/LangUtils.h"

#include "ExtractRes.h"
#include "ExtractDialogRes.h"

// #include "Help/Context/Extract.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

static const int kPathModeButtons[] =
{
  IDC_EXTRACT_RADIO_FULL_PATHNAMES,
  IDC_EXTRACT_RADIO_CURRENT_PATHNAMES,
  IDC_EXTRACT_RADIO_NO_PATHNAMES
};

#ifndef _SFX

static const NExtract::NPathMode::EEnum kPathModeButtonsVals[] =
{
  NExtract::NPathMode::kFullPathnames,
  NExtract::NPathMode::kCurrentPathnames,
  NExtract::NPathMode::kNoPathnames
};

static const int kNumPathnamesButtons = sizeof(kPathModeButtons) / sizeof(kPathModeButtons[0]);

static const int kOverwriteButtons[] =
{
  IDC_EXTRACT_RADIO_ASK_BEFORE_OVERWRITE,
  IDC_EXTRACT_RADIO_OVERWRITE_WITHOUT_PROMPT,
  IDC_EXTRACT_RADIO_SKIP_EXISTING_FILES,
  IDC_EXTRACT_RADIO_AUTO_RENAME,
  IDC_EXTRACT_RADIO_AUTO_RENAME_EXISTING,
};

static const NExtract::NOverwriteMode::EEnum kOverwriteButtonsVals[] =
{
  NExtract::NOverwriteMode::kAskBefore,
  NExtract::NOverwriteMode::kWithoutPrompt,
  NExtract::NOverwriteMode::kSkipExisting,
  NExtract::NOverwriteMode::kAutoRename,
  NExtract::NOverwriteMode::kAutoRenameExisting
};

static const int kNumOverwriteButtons = sizeof(kOverwriteButtons) / sizeof(kOverwriteButtons[0]);

/*
static const int kFilesButtons[] =
{
  IDC_EXTRACT_RADIO_SELECTED_FILES,
  IDC_EXTRACT_RADIO_ALL_FILES
};
static const int kNumFilesButtons = sizeof(kFilesButtons) / sizeof(kFilesButtons[0]);
*/

void CExtractDialog::GetPathMode()
{
  for (int i = 0; i < kNumPathnamesButtons; i++)
    if(IsButtonCheckedBool(kPathModeButtons[i]))
    {
      PathMode = kPathModeButtonsVals[i];
      return;
    }
  throw 1;
}

void CExtractDialog::SetPathMode()
{
  for (int j = 0; j < 2; j++)
  {
    for (int i = 0; i < kNumPathnamesButtons; i++)
      if(PathMode == kPathModeButtonsVals[i])
      {
        CheckRadioButton(kPathModeButtons[0], kPathModeButtons[kNumPathnamesButtons - 1], 
          kPathModeButtons[i]);
        return;
      }
    PathMode = kPathModeButtonsVals[0];
  }
  throw 1;
}

void CExtractDialog::GetOverwriteMode()
{
  for (int i = 0; i < kNumOverwriteButtons; i++)
    if(IsButtonCheckedBool(kOverwriteButtons[i]))
    {
      OverwriteMode = kOverwriteButtonsVals[i];
      return;
    }
  throw 0;
}

void CExtractDialog::SetOverwriteMode()
{
  for (int j = 0; j < 2; j++)
  {
    for (int i = 0; i < kNumOverwriteButtons; i++)
      if(OverwriteMode == kOverwriteButtonsVals[i])
      {
        CheckRadioButton(kOverwriteButtons[0], kOverwriteButtons[kNumOverwriteButtons - 1], 
            kOverwriteButtons[i]);
        return;
      }
    OverwriteMode = kOverwriteButtonsVals[0];
  }
  throw 1;
}

/*
int CExtractDialog::GetFilesMode() const
{
  for (int i = 0; i < kNumFilesButtons; i++)
    if(IsButtonCheckedBool(kFilesButtons[i]))
      return i;
  throw 0;
}
*/

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
  { IDC_EXTRACT_RADIO_AUTO_RENAME_EXISTING,     0x02000825 },
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

// static const int kWildcardsButtonIndex = 2;

#ifndef NO_REGISTRY
static const int kHistorySize = 8;
#endif

bool CExtractDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000800);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  #ifndef _SFX
  _passwordControl.Attach(GetItem(IDC_EXTRACT_EDIT_PASSWORD));
  _passwordControl.SetText(Password);
  _passwordControl.SetPasswordChar(TEXT('*'));
  #endif

  NExtract::CInfo extractionInfo;

  #ifdef NO_REGISTRY
  PathMode = NExtract::NPathMode::kFullPathnames;
  OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
  // extractionInfo.Paths = NExtract::NPathMode::kFullPathnames;
  #else
  ReadExtractionInfo(extractionInfo);
  CheckButton(IDC_EXTRACT_CHECK_SHOW_PASSWORD, extractionInfo.ShowPassword);
  UpdatePasswordControl();
  PathMode = extractionInfo.PathMode;
  OverwriteMode = extractionInfo.OverwriteMode;
  #endif

  _path.Attach(GetItem(IDC_EXTRACT_COMBO_PATH));

  _path.SetText(DirectoryPath);
  
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

  
  
  #ifndef _SFX
  SetPathMode();
  SetOverwriteMode();

  /*
  CheckRadioButton(kFilesButtons[0], kFilesButtons[kNumFilesButtons - 1], 
      kFilesButtons[_filesMode]);
  */

  // CWindow selectedFilesWindow = GetItem(IDC_EXTRACT_RADIO_SELECTED_FILES);
  // selectedFilesWindow.Enable(_enableSelectedFilesButton);


  #endif

 
  // CWindow filesWindow = GetItem(IDC_EXTRACT_RADIO_FILES);
  // filesWindow.Enable(_enableFilesButton);

  // UpdateWildCardState();
  return CModalDialog::OnInit();
}

#ifndef _SFX
void CExtractDialog::UpdatePasswordControl()
{
  _passwordControl.SetPasswordChar((IsButtonChecked(
    IDC_EXTRACT_CHECK_SHOW_PASSWORD) == BST_CHECKED) ? 0: TEXT('*'));
  UString password;
  _passwordControl.GetText(password);
  _passwordControl.SetText(password);
}
#endif

bool CExtractDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  /*
  for (int i = 0; i < kNumFilesButtons; i++)
    if (buttonID == kFilesButtons[i])
    {
      UpdateWildCardState();
      return true;
    }
  */
  switch(buttonID)
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
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CExtractDialog::OnButtonSetPath() 
{
  UString currentPath;
  _path.GetText(currentPath);
  UString title = LangStringSpec(IDS_EXTRACT_SET_FOLDER, 0x02000881);
  UString resultPath;
  if (!NShell::BrowseForFolder(HWND(*this), title, currentPath, resultPath))
    return;
  #ifndef NO_REGISTRY
  _path.SetCurSel(-1);
  #endif
  _path.SetText(resultPath);
}

void AddUniqueString(UStringVector &list, const UString &s)
{
  for(int i = 0; i < list.Size(); i++)
    if (s.CompareNoCase(list[i]) == 0)
      return;
  list.Add(s);
}

void CExtractDialog::OnOK() 
{
  #ifndef _SFX
  GetPathMode();
  GetOverwriteMode();
  // _filesMode = (NExtractionDialog::NFilesMode::EEnum)GetFilesMode();

  _passwordControl.GetText(Password);
  #endif

  NExtract::CInfo extractionInfo;
  extractionInfo.PathMode = PathMode;
  extractionInfo.OverwriteMode = OverwriteMode;
  extractionInfo.ShowPassword = (IsButtonChecked(
          IDC_EXTRACT_CHECK_SHOW_PASSWORD) == BST_CHECKED);
  
  UString s;
  
  #ifdef NO_REGISTRY
  
  _path.GetText(s);
  
  #else

  int currentItem = _path.GetCurSel();
  if(currentItem == CB_ERR)
  {
    _path.GetText(s);
    if(_path.GetCount() >= kHistorySize)
      currentItem = _path.GetCount() - 1;
  }
  else
    _path.GetLBText(currentItem, s);
  
  #endif

  s.Trim();
  #ifndef _SFX
  AddUniqueString(extractionInfo.Paths, s);
  #endif
  DirectoryPath = s;
  #ifndef  NO_REGISTRY
  for(int i = 0; i < _path.GetCount(); i++)
    if(i != currentItem)
    {
      UString sTemp;
      _path.GetLBText(i, sTemp);
      sTemp.Trim();
      AddUniqueString(extractionInfo.Paths, sTemp);
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

#ifndef  NO_REGISTRY
static LPCWSTR kHelpTopic = L"fm/plugins/7-zip/extract.htm";
void CExtractDialog::OnHelp() 
{
  ShowHelpWindow(NULL, kHelpTopic);
  CModalDialog::OnHelp();
}
#endif

