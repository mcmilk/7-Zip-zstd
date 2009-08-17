// LangPage.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/ResourceString.h"

#include "HelpUtils.h"
#include "LangPage.h"
#include "LangPageRes.h"
#include "LangUtils.h"
#include "RegistryUtils.h"

static CIDLangPair kIDLangPairs[] =
{
  { IDC_LANG_STATIC_LANG, 0x01000401}
};

static LPCWSTR kLangTopic = L"fm/options.htm#language";

static UString NativeLangString(const UString &s)
{
  return L" (" + s + L')';
}

bool CLangPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  _langCombo.Attach(GetItem(IDC_LANG_COMBO_LANG));

  UString s = NWindows::MyLoadStringW(IDS_LANG_ENGLISH) +
      NativeLangString(NWindows::MyLoadStringW(IDS_LANG_NATIVE));
  int index = (int)_langCombo.AddString(s);
  _langCombo.SetItemData(index, _paths.Size());
  _paths.Add(L"-");
  _langCombo.SetCurSel(0);

  CObjectVector<CLangEx> langs;
  LoadLangs(langs);
  for (int i = 0; i < langs.Size(); i++)
  {
    const CLangEx &lang = langs[i];
    UString name, nationalName;
    if (!lang.Lang.GetMessage(0, name))
      name = lang.ShortName;
    if (lang.Lang.GetMessage(1, nationalName) && !nationalName.IsEmpty())
      name += NativeLangString(nationalName);

    index = (int)_langCombo.AddString(name);
    _langCombo.SetItemData(index, _paths.Size());
    _paths.Add(lang.ShortName);
    if (g_LangID.CompareNoCase(lang.ShortName) == 0)
      _langCombo.SetCurSel(index);
  }
  return CPropertyPage::OnInit();
}

LONG CLangPage::OnApply()
{
  int selectedIndex = _langCombo.GetCurSel();
  int pathIndex = (int)_langCombo.GetItemData(selectedIndex);
  SaveRegLang(_paths[pathIndex]);
  ReloadLang();
  LangWasChanged = true;
  return PSNRET_NOERROR;
}

void CLangPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kLangTopic);
}

bool CLangPage::OnCommand(int code, int itemID, LPARAM param)
{
  if (code == CBN_SELCHANGE && itemID == IDC_LANG_COMBO_LANG)
  {
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, param);
}
