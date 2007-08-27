// EditPage.cpp

#include "StdAfx.h"
#include "EditPageRes.h"
#include "EditPage.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/CommonDialog.h"
// #include "Windows/FileFind.h"
// #include "Windows/FileDir.h"

#include "RegistryUtils.h"
#include "HelpUtils.h"
#include "LangUtils.h"
#include "ProgramLocation.h"

using namespace NWindows;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_EDIT_STATIC_EDITOR, 0x03010201}
};

static LPCWSTR kEditTopic = L"FM/options.htm#editor";

bool CEditPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  _editorEdit.Attach(GetItem(IDC_EDIT_EDIT_EDITOR));
  UString editorPath;
  ReadRegEditor(editorPath);
  _editorEdit.SetText(editorPath);
  return CPropertyPage::OnInit();
}

LONG CEditPage::OnApply()
{
  // int selectedIndex = _langCombo.GetCurSel();
  // int pathIndex = _langCombo.GetItemData(selectedIndex);
  // ReloadLang();
  UString editorPath;
  _editorEdit.GetText(editorPath);
  SaveRegEditor(editorPath);
  return PSNRET_NOERROR;
}

void CEditPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kEditTopic); // change it
}

bool CEditPage::OnButtonClicked(int aButtonID, HWND aButtonHWND)
{
  switch(aButtonID)
  {
    case IDC_EDIT_BUTTON_SET:
    {
      OnSetEditorButton();
      // if (!NShell::BrowseForFolder(HWND(*this), title, currentPath, aResultPath))
      //   return;
      return true;
    }
  }
  return CPropertyPage::OnButtonClicked(aButtonID, aButtonHWND);
}

void CEditPage::OnSetEditorButton()
{
  UString editorPath;
  _editorEdit.GetText(editorPath);
  UString resPath;
  if(!MyGetOpenFileName(HWND(*this), 0, editorPath, L"*.exe", resPath))
    return;
  _editorEdit.SetText(resPath);
  // Changed();
}

bool CEditPage::OnCommand(int code, int itemID, LPARAM param)
{
  if (code == EN_CHANGE && itemID == IDC_EDIT_EDIT_EDITOR)
  {
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, param);
}


