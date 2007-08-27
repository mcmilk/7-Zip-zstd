// FoldersPage.cpp

#include "StdAfx.h"

#include "FoldersPageRes.h"
#include "FoldersPage.h"

#include "Common/StringConvert.h"
#include "Windows/Defs.h"
#include "Windows/Shell.h"
#include "Windows/ResourceString.h"

#include "../Common/ZipRegistry.h"

#include "../FileManager/HelpUtils.h"
#include "../FileManager/LangUtils.h"

using namespace NWindows;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_FOLDERS_STATIC_WORKING_FOLDER,    0x01000210 },
  { IDC_FOLDERS_WORK_RADIO_SYSTEM,        0x01000211 },
  { IDC_FOLDERS_WORK_RADIO_CURRENT,       0x01000212 },
  { IDC_FOLDERS_WORK_RADIO_SPECIFIED,     0x01000213 },
  { IDC_FOLDERS_WORK_CHECK_FOR_REMOVABLE, 0x01000214 }
};

static const int kWorkModeButtons[] =
{
  IDC_FOLDERS_WORK_RADIO_SYSTEM,
  IDC_FOLDERS_WORK_RADIO_CURRENT,
  IDC_FOLDERS_WORK_RADIO_SPECIFIED
};

static const int kNumWorkModeButtons = sizeof(kWorkModeButtons) / sizeof(kWorkModeButtons[0]);
 
bool CFoldersPage::OnInit() 
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  ReadWorkDirInfo(m_WorkDirInfo);

  CheckButton(IDC_FOLDERS_WORK_CHECK_FOR_REMOVABLE, m_WorkDirInfo.ForRemovableOnly);
  
  CheckRadioButton(kWorkModeButtons[0], kWorkModeButtons[kNumWorkModeButtons - 1], 
      kWorkModeButtons[m_WorkDirInfo.Mode]);

  m_WorkPath.Init(*this, IDC_FOLDERS_WORK_EDIT_PATH);
  m_ButtonSetWorkPath.Init(*this, IDC_FOLDERS_WORK_BUTTON_PATH);

  m_WorkPath.SetText(m_WorkDirInfo.Path);

  MyEnableControls();
  
  return CPropertyPage::OnInit();
}

int CFoldersPage::GetWorkMode() const
{
  for (int i = 0; i < kNumWorkModeButtons; i++)
    if(IsButtonCheckedBool(kWorkModeButtons[i]))
      return i;
  throw 0;
}

void CFoldersPage::MyEnableControls()
{
  bool enablePath = (GetWorkMode() == NWorkDir::NMode::kSpecified);
  m_WorkPath.Enable(enablePath);
  m_ButtonSetWorkPath.Enable(enablePath);
}

void CFoldersPage::GetWorkDir(NWorkDir::CInfo &workDirInfo)
{
  m_WorkPath.GetText(workDirInfo.Path);
  workDirInfo.ForRemovableOnly = IsButtonCheckedBool(IDC_FOLDERS_WORK_CHECK_FOR_REMOVABLE);
  workDirInfo.Mode = NWorkDir::NMode::EEnum(GetWorkMode());
}

/*
bool CFoldersPage::WasChanged()
{
  NWorkDir::CInfo workDirInfo;
  GetWorkDir(workDirInfo);
  return (workDirInfo.Mode != m_WorkDirInfo.Mode ||
      workDirInfo.ForRemovableOnly != m_WorkDirInfo.ForRemovableOnly ||
      workDirInfo.Path.Compare(m_WorkDirInfo.Path) != 0);
}
*/

void CFoldersPage::ModifiedEvent()
{
  Changed();
  /*
  if (WasChanged())
    Changed();
  else
    UnChanged();
  */
}

bool CFoldersPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{ 
  for (int i = 0; i < kNumWorkModeButtons; i++)
    if (buttonID == kWorkModeButtons[i])
    {
      MyEnableControls();
      ModifiedEvent();
      return true;
    }
  switch(buttonID)
  {
    case IDC_FOLDERS_WORK_BUTTON_PATH:
      OnFoldersWorkButtonPath();
      break;
    case IDC_FOLDERS_WORK_CHECK_FOR_REMOVABLE:
      break;
    default:
      return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
  }
  ModifiedEvent();
  return true;
}

bool CFoldersPage::OnCommand(int code, int itemID, LPARAM lParam)
{
  if (code == EN_CHANGE && itemID == IDC_FOLDERS_WORK_EDIT_PATH)
  {
    ModifiedEvent();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, lParam);
}

void CFoldersPage::OnFoldersWorkButtonPath() 
{
  UString currentPath;
  m_WorkPath.GetText(currentPath);
  UString title = LangString(IDS_FOLDERS_SET_WORK_PATH_TITLE, 0x01000281);
  UString resultPath;
  if (NShell::BrowseForFolder(HWND(*this), title, currentPath, resultPath))
    m_WorkPath.SetText(resultPath);
}

LONG CFoldersPage::OnApply() 
{
  GetWorkDir(m_WorkDirInfo);
  SaveWorkDirInfo(m_WorkDirInfo);
  return PSNRET_NOERROR;
}

static LPCWSTR kFoldersTopic = L"fm/plugins/7-zip/options.htm#folders";

void CFoldersPage::OnNotifyHelp() 
{
  ShowHelpWindow(NULL, kFoldersTopic);
}
