// EditPage.cpp

#include "StdAfx.h"

#include "EditPage.h"
#include "EditPageRes.h"

#include "BrowseDialog.h"
#include "HelpUtils.h"
#include "LangUtils.h"
#include "RegistryUtils.h"

using namespace NWindows;

static const UInt32 kLangIDs[] =
{
  IDT_EDIT_EDITOR,
  IDT_EDIT_DIFF
};

static const UInt32 kLangIDs_Colon[] =
{
  IDT_EDIT_VIEWER
};

static LPCWSTR kEditTopic = L"FM/options.htm#editor";

bool CEditPage::OnInit()
{
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));
  LangSetDlgItems_Colon(*this, kLangIDs_Colon, ARRAY_SIZE(kLangIDs_Colon));

  _viewer.Attach(GetItem(IDE_EDIT_VIEWER));
  _editor.Attach(GetItem(IDE_EDIT_EDITOR));
  _diff.Attach(GetItem(IDE_EDIT_DIFF));
  
  {
    UString path;
    ReadRegEditor(false, path);
    _viewer.SetText(path);
  }
  {
    UString path;
    ReadRegEditor(true, path);
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
    _viewer.GetText(path);
    SaveRegEditor(false, path);
  }
  {
    UString path;
    _editor.GetText(path);
    SaveRegEditor(true, path);
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
  if (MyBrowseForFile(hwnd, 0, path, NULL, L"*.exe", resPath))
  {
    edit.SetText(resPath);
    // Changed();
  }
}

bool CEditPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    case IDB_EDIT_VIEWER: Edit_BrowseForFile(_viewer, *this); return true;
    case IDB_EDIT_EDITOR: Edit_BrowseForFile(_editor, *this); return true;
    case IDB_EDIT_DIFF:   Edit_BrowseForFile(_diff, *this); return true;
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
}

bool CEditPage::OnCommand(int code, int itemID, LPARAM param)
{
  if (code == EN_CHANGE && (
      itemID == IDE_EDIT_VIEWER ||
      itemID == IDE_EDIT_EDITOR ||
      itemID == IDE_EDIT_DIFF))
  {
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, param);
}
