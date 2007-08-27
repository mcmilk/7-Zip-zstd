// LangPage.cpp

#include "StdAfx.h"
#include "LangPageRes.h"
#include "LangPage.h"

#include "Common/StringConvert.h"

#include "Windows/ResourceString.h"

#include "RegistryUtils.h"
#include "HelpUtils.h"
#include "LangUtils.h"

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_LANG_STATIC_LANG, 0x01000401}
};

static LPCWSTR kLangTopic = L"fm/options.htm#language";

bool CLangPage::OnInit()
{
  _langWasChanged = false;
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  _langCombo.Attach(GetItem(IDC_LANG_COMBO_LANG));

  UString s = NWindows::MyLoadStringW(IDS_LANG_ENGLISH);
  s += L" (";
  s += NWindows::MyLoadStringW(IDS_LANG_NATIVE);
  s += L")";
  int index = (int)_langCombo.AddString(s);
  _langCombo.SetItemData(index, _paths.Size());
  _paths.Add(L"-");
  _langCombo.SetCurSel(0);

  CObjectVector<CLangEx> langs;
  LoadLangs(langs);
  for (int i = 0; i < langs.Size(); i++)
  {
    const CLangEx &lang = langs[i];
    UString name; 
    UString englishName, nationalName;
    if (lang.Lang.GetMessage(0x00000000, englishName))
      name = englishName;
    else
      name = lang.ShortName;
    if (lang.Lang.GetMessage(0x00000001, nationalName))
    {
      if (!nationalName.IsEmpty())
      {
        name += L" (";
        name += nationalName;
        name += L")";
      }
    }
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
  _langWasChanged = true;
  return PSNRET_NOERROR;
}

void CLangPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kLangTopic); // change it
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
