// SystemDialog.cpp

#include "StdAfx.h"
#include "resource.h"
#include "SystemPage.h"

#include "../Common/ZipRegistry.h"
#include "../Common/HelpUtils.h"
#include "../Common/ZipRegistryConfig.h"
#include "../Common/LangUtils.h"

#include "Windows/Defs.h"
#include "Windows/Control/ListView.h"


using namespace NZipSettings;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, 0x01000301},
  { IDC_STATIC_SYSTEM_ASSOCIATE,          0x01000302}
};

static LPCTSTR kSystemTopic = _T("gui/7-zipCfg/system.htm");


bool CSystemPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  m_ListView.Attach(GetItem(IDC_SYSTEM_LIST_ASSOCIATE));

  CheckButton(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, 
      NZipRootRegistry::CheckContextMenuHandler());

  UINT32 aNewFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  m_ListView.SetExtendedListViewStyle(aNewFlags, aNewFlags);

  // LVCOLUMN aColumn;
  // aColumn.mask = LVCF_WIDTH;
  // aColumn.cx = 130;
  // m_ListView.InsertColumn(0, &aColumn);

  CZipRegistryManager aRegistryManager;
  NZipRootRegistry::ReadArchiverInfoList(m_Archivers);
  for (int i = 0; i < m_Archivers.Size(); i++)
  {
    const NZipRootRegistry::CArchiverInfo &anArchiverInfo = m_Archivers[i];
    LVITEM anItem;
    anItem.iItem = i;
    anItem.mask = LVIF_TEXT;
    anItem.pszText = (LPTSTR)(LPCTSTR)anArchiverInfo.Name;
    anItem.iSubItem = 0;
    m_ListView.InsertItem(&anItem);

    bool aCheck = NZipRootRegistry::CheckShellExtensionInfo(anArchiverInfo.Extension);
    m_ListView.SetCheckState(i, aCheck);
  }
  if(m_ListView.GetItemCount() > 0)
  {
    UINT aState = LVIS_SELECTED | LVIS_FOCUSED;
    m_ListView.SetItemState(0, aState, aState);
  }
  return CPropertyPage::OnInit();
}

LONG CSystemPage::OnApply()
{
  for (int i = 0; i < m_Archivers.Size(); i++)
  {
    const NZipRootRegistry::CArchiverInfo &anArchiverInfo = m_Archivers[i];
    CSysString anArchiveTitle = anArchiverInfo.Name;
    anArchiveTitle += _T(" ");
    anArchiveTitle += _T("Archive");
    if (m_ListView.GetCheckState(i))
      NZipRootRegistry::AddShellExtensionInfo(anArchiverInfo.Extension, 
          anArchiveTitle, anArchiverInfo.ClassID, NULL, 0);
    else
      NZipRootRegistry::DeleteShellExtensionInfo(anArchiverInfo.Extension);
  }
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
  if (lParam->hwndFrom == HWND(m_ListView) && lParam->code == LVN_ITEMCHANGED)
  {
  	const NMLISTVIEW *aNMListView = (const NMLISTVIEW *)lParam;
    if ((aNMListView->uChanged & LVIF_STATE) != 0)
    {
      UINT anOldState = aNMListView->uOldState & LVIS_STATEIMAGEMASK;
      UINT aNewState = aNMListView->uNewState & LVIS_STATEIMAGEMASK;
      if (anOldState != aNewState)
        Changed();
    }
    return true;
  }
  return CPropertyPage::OnNotify(aControlID, lParam); 
}


/*
static LPCTSTR kZIPExtension = _T("zip");
static LPCTSTR kRARExtension = _T("rar");

static BYTE kZipShellNewData[] =
  { 0x50-1, 0x4B, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0 };

static BYTE kRarShellNewData[] =
  { 0x52-1, 0x61, 0x72, 0x21, 0x1A, 7, 0, 0xCF, 0x90, 0x73, 0, 0, 0x0D, 0, 0, 0, 0, 0, 0, 0};

class CSignatureMaker
{
public:
  CSignatureMaker()
  {
    kZipShellNewData[0]++;
    kRarShellNewData[0]++;
  };
};

static CSignatureMaker g_SignatureMaker;
*/
