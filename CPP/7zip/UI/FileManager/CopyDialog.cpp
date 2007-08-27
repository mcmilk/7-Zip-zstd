// CopyDialog.cpp

#include "StdAfx.h"
#include "CopyDialog.h"

#include "Common/StringConvert.h"

#include "Windows/Control/Static.h"
#include "Windows/Shell.h"
#include "Windows/FileName.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDOK, 0x02000702 },
  { IDCANCEL, 0x02000710 }
};
#endif

bool CCopyDialog::OnInit() 
{
  #ifdef LANG        
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _path.Attach(GetItem(IDC_COPY_COMBO));
  SetText(Title);

  NControl::CStatic staticContol;
  staticContol.Attach(GetItem(IDC_COPY_STATIC));
  staticContol.SetText(Static);
  for(int i = 0; i < Strings.Size(); i++)
    _path.AddString(Strings[i]);
  _path.SetText(Value);
  return CModalDialog::OnInit();
}

bool CCopyDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDC_COPY_SET_PATH:
      OnButtonSetPath();
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CCopyDialog::OnButtonSetPath() 
{
  UString currentPath;
  _path.GetText(currentPath);

  /*
  #ifdef LANG        
  UString title = LangLoadString(IDS_EXTRACT_SET_FOLDER, 0x02000881);
  #else
  UString title = MyLoadString(IDS_EXTRACT_SET_FOLDER);
  #endif
  */
  UString title = LangStringSpec(IDS_SET_FOLDER, 0x03020209);
  // UString title = L"Specify a location for output folder";

  UString resultPath;
  if (!NShell::BrowseForFolder(HWND(*this), title, currentPath, resultPath))
    return;
  NFile::NName::NormalizeDirPathPrefix(resultPath);
  _path.SetCurSel(-1);
  _path.SetText(resultPath);
}

void CCopyDialog::OnOK()
{
  _path.GetText(Value);
  CModalDialog::OnOK();
}
