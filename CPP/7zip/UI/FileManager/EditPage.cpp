// EditPage.cpp

#include "StdAfx.h"

#include "EditPage.h"
#include "EditPageRes.h"

#include "BrowseDialog.h"
#include "HelpUtils.h"
#include "LangUtils.h"
#include "RegistryUtils.h"

using namespace NWindows;

static CIDLangPair kIDLangPairs[] =
{
  { IDC_EDIT_STATIC_EDITOR, 0x03010201},
  { IDC_EDIT_STATIC_DIFF, 0x03010202}
};

static LPCWSTR kEditTopic = L"FM/options.htm#editor";

bool CEditPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  _editor.Attach(GetItem(IDC_EDIT_EDIT_EDITOR));
  _diff.Attach(GetItem(IDC_EDIT_EDIT_DIFF));
  
  {
    UString path;
    ReadRegEditor(path);
    _editor.SetText(path);
  }
  {
    UString path;
    ReadRegDiff(path);
    _diff.SetText(path);
  }
  return CPropertyPage::OnInit();
}

LONG CEditPage::OnApply()
{
  {
    UString path;
    _editor.GetText(path);
    SaveRegEditor(path);
  }
  {
    UString path;
    _diff.GetText(path);
    SaveRegDiff(path);
  }
  return PSNRET_NOERROR;
}

void CEditPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kEditTopic);
}

static void Edit_BrowseForFile(NWindows::NControl::CEdit &edit, HWND hwnd)
{
  UString path;
  edit.GetText(path);
  UString resPath;
  if (MyBrowseForFile(hwnd, 0, path, L"*.exe", resPath))
  {
    edit.SetText(resPath);
    // Changed();
  }
}

bool CEditPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    case IDC_EDIT_BUTTON_EDITOR:
      Edit_BrowseForFile(_editor, *this);
      return true;
    case IDC_EDIT_BUTTON_DIFF:
      Edit_BrowseForFile(_diff, *this);
      return true;
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
}

bool CEditPage::OnCommand(int code, int itemID, LPARAM param)
{
  if (code == EN_CHANGE &&
      (itemID == IDC_EDIT_EDIT_EDITOR ||
      itemID == IDC_EDIT_EDIT_DIFF))
  {
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, param);
}
