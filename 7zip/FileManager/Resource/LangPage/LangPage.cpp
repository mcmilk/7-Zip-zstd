// LangPage.cpp

#include "StdAfx.h"
#include "resource.h"
#include "LangPage.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/FileFind.h"

#include "../../RegistryUtils.h"
#include "../../HelpUtils.h"
#include "../../LangUtils.h"
#include "../../ProgramLocation.h"

#include "../../MyLoadMenu.h"

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_LANG_STATIC_LANG, 0x01000401}
};

static LPCWSTR kLangTopic = L"FM/options.htm#language";

bool CLangPage::OnInit()
{
  _langWasChanged = false;
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  _langCombo.Attach(GetItem(IDC_LANG_COMBO_LANG));

  int index = _langCombo.AddString(TEXT("English (English)"));
  _langCombo.SetItemData(index, _paths.Size());
  _paths.Add(TEXT(""));
  _langCombo.SetCurSel(0);

  UString folderPath;
  if (::GetProgramFolderPath(folderPath))
  {
    folderPath += L"Lang\\";
    NWindows::NFile::NFind::CEnumeratorW enumerator(folderPath + L"*.txt");
    NWindows::NFile::NFind::CFileInfoW fileInfo;
    while (enumerator.Next(fileInfo))
    {
      if (fileInfo.IsDirectory())
        continue;
      CLang lang;
      UString filePath = folderPath + fileInfo.Name;
      if (lang.Open(GetSystemString(filePath)))
      {
        UString name; 
        UString englishName, nationalName;
        if (lang.GetMessage(0x00000000, englishName))
          name += englishName;
        if (lang.GetMessage(0x00000001, nationalName))
        {
          if (!nationalName.IsEmpty())
          {
            name += L" (";
            name += nationalName;
            name += L")";
          }
        }
        if (name.IsEmpty())
          name = fileInfo.Name;
        index = _langCombo.AddString(GetSystemString(name));
        _langCombo.SetItemData(index, _paths.Size());
        _paths.Add(GetSystemString(filePath));
        if (g_LangPath.CollateNoCase(GetSystemString(filePath)) == 0)
          _langCombo.SetCurSel(index);
      }
    }
  }
  return CPropertyPage::OnInit();
}

LONG CLangPage::OnApply()
{
  int selectedIndex = _langCombo.GetCurSel();
  int pathIndex = _langCombo.GetItemData(selectedIndex);
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

