// SplitDialog.cpp

#include "StdAfx.h"
#include "SplitDialog.h"

#include "Common/StringToInt.h"
#include "Windows/Shell.h"
#include "Windows/FileName.h"

#include "../../SplitUtils.h"
#ifdef LANG        
#include "../../LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDC_STATIC_SPLIT_PATH, 0x03020501 },
  { IDC_STATIC_SPLIT_VOLUME, 0x02000D40 },
};
#endif


bool CSplitDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x03020500);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _pathCombo.Attach(GetItem(IDC_COMBO_SPLIT_PATH));
  _volumeCombo.Attach(GetItem(IDC_COMBO_SPLIT_VOLUME));
  
  if (!FilePath.IsEmpty())
  {
    UString title;
    GetText(title);
    title += ' ';
    title += FilePath;
    SetText(title);
  }
  _pathCombo.SetText(Path);
  AddVolumeItems(_volumeCombo);
  _volumeCombo.SetCurSel(0);
  return CModalDialog::OnInit();
}

bool CSplitDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDC_BUTTON_SPLIT_PATH:
      OnButtonSetPath();
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CSplitDialog::OnButtonSetPath() 
{
  CSysString currentPath;
  _pathCombo.GetText(currentPath);
  CSysString title = TEXT("Specify a location for output folder");
  CSysString resultPath;
  if (!NShell::BrowseForFolder(HWND(*this), title, currentPath, resultPath))
    return;
  NFile::NName::NormalizeDirPathPrefix(resultPath);
  _pathCombo.SetCurSel(-1);
  _pathCombo.SetText(resultPath);
}

void CSplitDialog::OnOK()
{
  _pathCombo.GetText(Path);
  UString volumeString;
  _volumeCombo.GetText(volumeString);
  volumeString.Trim();
  if (!ParseVolumeSizes(volumeString, VolumeSizes))
  {
    MessageBox((HWND)*this, TEXT("Incorrect volume size"), TEXT("7-Zip"), MB_ICONERROR);
    return;
  }
  CModalDialog::OnOK();
}
