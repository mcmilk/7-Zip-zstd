// MenuPage.cpp

#include "StdAfx.h"

#include "../Common/ZipRegistry.h"

#include "../Explorer/ContextMenuFlags.h"
#include "../Explorer/RegistryContextMenu.h"
#include "../Explorer/resource.h"

#include "HelpUtils.h"
#include "LangUtils.h"
#include "MenuPage.h"
#include "MenuPageRes.h"
#include "FormatUtils.h"

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
  { IDS_CONTEXT_OPEN, 0x02000103, kOpenAs},
  { IDS_CONTEXT_EXTRACT, 0x02000105, kExtract},
  { IDS_CONTEXT_EXTRACT_HERE, 0x0200010B, kExtractHere },
  { IDS_CONTEXT_EXTRACT_TO, 0x0200010D, kExtractTo },

  { IDS_CONTEXT_TEST, 0x02000109, kTest},

  { IDS_CONTEXT_COMPRESS, 0x02000107, kCompress },
  { IDS_CONTEXT_COMPRESS_TO, 0x0200010F, kCompressTo7z },
  { IDS_CONTEXT_COMPRESS_TO, 0x0200010F, kCompressToZip }

  #ifndef UNDER_CE
  ,
  { IDS_CONTEXT_COMPRESS_EMAIL, 0x02000111, kCompressEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, 0x02000113, kCompressTo7zEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, 0x02000113, kCompressToZipEmail }
  #endif
};

const int kNumMenuItems = sizeof(kMenuItems) / sizeof(kMenuItems[0]);

bool CMenuPage::OnInit()
{
  _initMode = true;
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  #ifdef UNDER_CE
  EnableItem(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, false);
  #else
  CheckButton(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, NZipRootRegistry::CheckContextMenuHandler());
  #endif

  CContextMenuInfo ci;
  ci.Load();

  CheckButton(IDC_SYSTEM_CASCADED_MENU, ci.Cascaded);

  _listView.Attach(GetItem(IDC_SYSTEM_OPTIONS_LIST));

  UInt32 newFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  _listView.SetExtendedListViewStyle(newFlags, newFlags);

  _listView.InsertColumn(0, L"", 100);

  for (int i = 0; i < kNumMenuItems; i++)
  {
    CContextMenuItem &menuItem = kMenuItems[i];

    UString s = LangString(menuItem.ControlID, menuItem.LangID);
    if (menuItem.Flag == kOpenAs)
      s += L" >";

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

    int itemIndex = _listView.InsertItem(i, s);
    _listView.SetCheckState(itemIndex, ((ci.Flags & menuItem.Flag) != 0));
  }

  _listView.SetColumnWidthAuto(0);
  _initMode = false;
  return CPropertyPage::OnInit();
}

#ifndef UNDER_CE
STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
HWND g_MenuPageHWND = 0;
#endif

LONG CMenuPage::OnApply()
{
  #ifndef UNDER_CE
  g_MenuPageHWND = *this;
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
  #endif

  CContextMenuInfo ci;
  ci.Cascaded = IsButtonCheckedBool(IDC_SYSTEM_CASCADED_MENU);
  ci.Flags = 0;
  for (int i = 0; i < kNumMenuItems; i++)
    if (_listView.GetCheckState(i))
      ci.Flags |= kMenuItems[i].Flag;
  ci.Save();

  return PSNRET_NOERROR;
}

void CMenuPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kSystemTopic);
}

bool CMenuPage::OnButtonClicked(int buttonID, HWND buttonHWND)
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

bool CMenuPage::OnNotify(UINT controlID, LPNMHDR lParam)
{
  if (lParam->hwndFrom == HWND(_listView))
  {
    switch(lParam->code)
    {
      case (LVN_ITEMCHANGED):
        return OnItemChanged((const NMLISTVIEW *)lParam);
    }
  }
  return CPropertyPage::OnNotify(controlID, lParam);
}


bool CMenuPage::OnItemChanged(const NMLISTVIEW *info)
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
  return true;
}
