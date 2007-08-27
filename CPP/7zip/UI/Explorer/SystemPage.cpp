// SystemPage.cpp

#include "StdAfx.h"

#include "resource.h"
#include "SystemPageRes.h"
#include "SystemPage.h"

#include "Common/StringConvert.h"
#include "Windows/Defs.h"
#include "Windows/Control/ListView.h"

#include "../Common/ZipRegistry.h"

#include "../FileManager/HelpUtils.h"
#include "../FileManager/LangUtils.h"
#include "../FileManager/FormatUtils.h"

#include "RegistryContextMenu.h"
#include "ContextMenuFlags.h"

using namespace NContextMenuFlags;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, 0x01000301},
  { IDC_SYSTEM_CASCADED_MENU, 0x01000302},
  { IDC_SYSTEM_STATIC_CONTEXT_MENU_ITEMS, 0x01000310}
};

static LPCWSTR kSystemTopic = L"fm/plugins/7-zip/options.htm#system";

struct CContextMenuItem
{
  int ControlID;
  UInt32 LangID;
  UInt32 Flag;
};

static CContextMenuItem kMenuItems[] = 
{
  { IDS_CONTEXT_OPEN, 0x02000103, kOpen},
  { IDS_CONTEXT_EXTRACT, 0x02000105, kExtract},
  { IDS_CONTEXT_EXTRACT_HERE, 0x0200010B, kExtractHere },
  { IDS_CONTEXT_EXTRACT_TO, 0x0200010D, kExtractTo },

  { IDS_CONTEXT_TEST, 0x02000109, kTest},

  { IDS_CONTEXT_COMPRESS, 0x02000107, kCompress },
  { IDS_CONTEXT_COMPRESS_EMAIL, 0x02000111, kCompressEmail },
  { IDS_CONTEXT_COMPRESS_TO, 0x0200010F,  kCompressTo7z },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, 0x02000113, kCompressTo7zEmail},
  { IDS_CONTEXT_COMPRESS_TO, 0x0200010F,  kCompressToZip },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, 0x02000113, kCompressToZipEmail}
};

const int kNumMenuItems = sizeof(kMenuItems) / sizeof(kMenuItems[0]);

bool CSystemPage::OnInit()
{
  _initMode = true;
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  CheckButton(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, 
      NZipRootRegistry::CheckContextMenuHandler());

  CheckButton(IDC_SYSTEM_CASCADED_MENU, ReadCascadedMenu());

  UInt32 contextMenuFlags;
  if (!ReadContextMenuStatus(contextMenuFlags))
    contextMenuFlags = NContextMenuFlags::GetDefaultFlags();

  m_ListView.Attach(GetItem(IDC_SYSTEM_OPTIONS_LIST));

  /*
  CheckButton(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, 
      NRegistryAssociations::CheckContextMenuHandler());
  */

  UInt32 newFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  m_ListView.SetExtendedListViewStyle(newFlags, newFlags);

  UString s; //  = TEXT("Items"); // LangLoadString(IDS_PROPERTY_EXTENSION, 0x02000205);
  LVCOLUMNW column;
  column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT | LVCF_SUBITEM;
  column.cx = 270;
  column.fmt = LVCFMT_LEFT;
  column.pszText = (LPWSTR)(LPCWSTR)s;
  column.iSubItem = 0;
  m_ListView.InsertColumn(0, &column);

  for (int i = 0; i < kNumMenuItems; i++)
  {
    CContextMenuItem &menuItem = kMenuItems[i];
    LVITEMW item;
    item.iItem = i;
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.lParam = i;

    UString s = LangString(menuItem.ControlID, menuItem.LangID);

    switch(menuItem.ControlID)
    {
      case IDS_CONTEXT_EXTRACT_TO:
      {
        s = MyFormatNew(s, LangString(IDS_CONTEXT_FOLDER, 0x02000140));
        break;
      }
      case IDS_CONTEXT_COMPRESS_TO:
      case IDS_CONTEXT_COMPRESS_TO_EMAIL:
      {
        UString s2 = LangString(IDS_CONTEXT_ARCHIVE, 0x02000141);
        switch(menuItem.Flag)
        {
          case kCompressTo7z:
          case kCompressTo7zEmail:
            s2 += L".7z";
            break;
          case kCompressToZip:
          case kCompressToZipEmail:
            s2 += L".zip";
            break;
        }
        s = MyFormatNew(s, s2);
        break;
      }
    }

    // UString MyFormatNew(const UString &format, const UString &argument);

    item.pszText = (LPWSTR)(LPCWSTR)s;
    item.iSubItem = 0;
    int itemIndex = m_ListView.InsertItem(&item);
    m_ListView.SetCheckState(itemIndex, ((contextMenuFlags & menuItem.Flag) != 0));
  }

  _initMode = false;
  return CPropertyPage::OnInit();
}

STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);

LONG CSystemPage::OnApply()
{
  if (IsButtonCheckedBool(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU))
  {
    DllRegisterServer();
    NZipRootRegistry::AddContextMenuHandler();
  }
  else
  {
    DllUnregisterServer();
    NZipRootRegistry::DeleteContextMenuHandler();
  }
  SaveCascadedMenu(IsButtonCheckedBool(IDC_SYSTEM_CASCADED_MENU));

  UInt32 flags = 0;
  for (int i = 0; i < kNumMenuItems; i++)
    if (m_ListView.GetCheckState(i))
      flags |= kMenuItems[i].Flag;
  SaveContextMenuStatus(flags);

  return PSNRET_NOERROR;
}

void CSystemPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kSystemTopic);
}

bool CSystemPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{ 
  switch(buttonID)
  {
    case IDC_SYSTEM_CASCADED_MENU:
    case IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU:
      Changed();
      return true;
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);

}

bool CSystemPage::OnNotify(UINT controlID, LPNMHDR lParam) 
{ 
  if (lParam->hwndFrom == HWND(m_ListView))
  {
    switch(lParam->code)
    {
      case (LVN_ITEMCHANGED):
        return OnItemChanged((const NMLISTVIEW *)lParam);
    }
  } 
  return CPropertyPage::OnNotify(controlID, lParam); 
}


bool CSystemPage::OnItemChanged(const NMLISTVIEW *info)
{
  if (_initMode)
    return true;
  if ((info->uChanged & LVIF_STATE) != 0)
  {
    UINT oldState = info->uOldState & LVIS_STATEIMAGEMASK;
    UINT newState = info->uNewState & LVIS_STATEIMAGEMASK;
    if (oldState != newState)
      Changed();
  }
  // PostMessage(kRefreshpluginsListMessage, 0);
  // RefreshPluginsList();
  return true;
}
