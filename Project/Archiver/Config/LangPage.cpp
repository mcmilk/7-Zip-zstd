// SystemDialog.cpp

#include "StdAfx.h"
#include "resource.h"
#include "LangPage.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/Control/ListView.h"
#include "Windows/FileFind.h"

#include "../Common/ZipRegistry.h"
#include "../Common/HelpUtils.h"
#include "../Common/LangUtils.h"

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_STATIC_LANG_LANG, 0x01000401}
};

static LPCTSTR kSystemTopic = _T("gui/7-zipCfg/lang.htm");

bool CLangPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  m_Lang.Attach(GetItem(IDC_COMBO_LANG_LANG));

  int anIndex = m_Lang.AddString(TEXT("English (English)"));
  m_Lang.SetItemData(anIndex, m_Paths.Size());
  m_Paths.Add(TEXT(""));
  m_Lang.SetCurSel(0);

  CSysString aFolderPath;
  if (GetProgramDirPrefix(aFolderPath))
  {
    aFolderPath += TEXT("Lang\\");
    NWindows::NFile::NFind::CEnumerator anEnumerator(aFolderPath + TEXT("*.txt"));
    NWindows::NFile::NFind::CFileInfo aFileInfo;
    while (anEnumerator.Next(aFileInfo))
    {
      if (aFileInfo.IsDirectory())
        continue;
      CLang aLang;
      CSysString aFilePath = aFolderPath + aFileInfo.Name;
      if (aLang.Open(aFilePath))
      {
        CSysString aName; 
        UString anEnglishName, aNationalName;
        if (aLang.GetMessage(0x00000000, anEnglishName))
          aName += GetSystemString(anEnglishName);
        if (aLang.GetMessage(0x00000001, aNationalName))
        {
          if (!aNationalName.IsEmpty())
          {
            aName += TEXT(" (");
            aName += GetSystemString(aNationalName);
            aName += TEXT(")");
          }
        }
        if (aName.IsEmpty())
          aName = aFileInfo.Name;
        anIndex = m_Lang.AddString(aName);
        m_Lang.SetItemData(anIndex, m_Paths.Size());
        m_Paths.Add(aFilePath);
        if (aLangPath.CollateNoCase(aFilePath) == 0)
          m_Lang.SetCurSel(anIndex);
      }
    }
  }
  return CPropertyPage::OnInit();
}

LONG CLangPage::OnApply()
{
  int aSelectedIndex = m_Lang.GetCurSel();
  int aPathIndex = m_Lang.GetItemData(aSelectedIndex);
  SaveRegLang(m_Paths[aPathIndex]);
  return PSNRET_NOERROR;
}

void CLangPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kSystemTopic); // change it
}

bool CLangPage::OnCommand(int aCode, int anItemID, LPARAM lParam)
{
  if (aCode == CBN_SELCHANGE && anItemID == IDC_COMBO_LANG_LANG)
  {
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(aCode, anItemID, lParam);
}

