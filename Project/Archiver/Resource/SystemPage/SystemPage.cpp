// SystemDialog.cpp

#include "StdAfx.h"
#include "resource.h"
#include "SystemPage.h"

#include "../../Common/ZipRegistry.h"
#include "../../Common/ZipRegistryConfig.h"

#include "../../../FileManager/HelpUtils.h"
#include "../../../FileManager/LangUtils.h"

#include "Windows/Defs.h"
#include "Windows/Control/ListView.h"


using namespace NZipSettings;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, 0x01000301}
};

static LPCTSTR kSystemTopic = _T("gui/7-zipCfg/system.htm");


bool CSystemPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  CheckButton(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, 
      NZipRootRegistry::CheckContextMenuHandler());

  return CPropertyPage::OnInit();
}

LONG CSystemPage::OnApply()
{
  if (IsButtonCheckedBool(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU))
    NZipRootRegistry::AddContextMenuHandler();
  else
    NZipRootRegistry::DeleteContextMenuHandler();
  return PSNRET_NOERROR;
}

void CSystemPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kSystemTopic);
}

bool CSystemPage::OnButtonClicked(int aButtonID, HWND aButtonHWND)
{ 
  switch(aButtonID)
  {
    case IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU:
      Changed();
      return true;
  }
  return CPropertyPage::OnButtonClicked(aButtonID, aButtonHWND);
}

bool CSystemPage::OnNotify(UINT aControlID, LPNMHDR lParam) 
{ 
  return CPropertyPage::OnNotify(aControlID, lParam); 
}

