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
  UString editorPath;
  _editorEdit.GetText(editorPath);
  SaveRegEditor(editorPath);
  return PSNRET_NOERROR;
}

void CEditPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kEditTopic);
}

bool CEditPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    case IDC_EDIT_BUTTON_SET:
    {
      UString editorPath;
      _editorEdit.GetText(editorPath);
      UString resPath;
      if (MyBrowseForFile(HWND(*this), 0, editorPath, L"*.exe", resPath))
      {
        _editorEdit.SetText(resPath);
        // Changed();
      }
      return true;
    }
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
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
