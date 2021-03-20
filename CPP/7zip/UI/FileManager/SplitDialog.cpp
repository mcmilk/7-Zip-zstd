// SplitDialog.cpp

#include "StdAfx.h"

#include "../../../Windows/FileName.h"

#ifdef LANG
#include "LangUtils.h"
#endif

#include "BrowseDialog.h"
#include "CopyDialogRes.h"
#include "SplitDialog.h"
#include "SplitUtils.h"
#include "resourceGui.h"

using namespace NWindows;

#ifdef LANG
static const UInt32 kLangIDs[] =
{
  IDT_SPLIT_PATH,
  IDT_SPLIT_VOLUME
};
#endif


bool CSplitDialog::OnInit()
{
  #ifdef LANG
  LangSetWindowText(*this, IDD_SPLIT);
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  #endif
  _pathCombo.Attach(GetItem(IDC_SPLIT_PATH));
  _volumeCombo.Attach(GetItem(IDC_SPLIT_VOLUME));
  
  if (!FilePath.IsEmpty())
  {
    UString title;
    GetText(title);
    title.Add_Space();
    title += FilePath;
    SetText(title);
  }
  _pathCombo.SetText(Path);
  AddVolumeItems(_volumeCombo);
  _volumeCombo.SetCurSel(0);
  NormalizeSize();
  return CModalDialog::OnInit();
}


bool CSplitDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    case IDB_SPLIT_PATH:
      OnButtonSetPath();
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CSplitDialog::OnButtonSetPath()
{
  UString currentPath;
  _pathCombo.GetText(currentPath);
  // UString title = "Specify a location for output folder";
  UString title = LangString(IDS_SET_FOLDER);

  UString resultPath;
  if (!MyBrowseForFolder(*this, title, currentPath, resultPath))
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
  if (!ParseVolumeSizes(volumeString, VolumeSizes) || VolumeSizes.Size() == 0)
  {
    ::MessageBoxW(*this, LangString(IDS_INCORRECT_VOLUME_SIZE), L"7-Zip", MB_ICONERROR);
    return;
  }
  CModalDialog::OnOK();
}
