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

#include "../FileManager/PropertyNameRes.h"

using namespace NContextMenuFlags;

static const UInt32 kLangIDs[] =
{
  IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU,
  IDX_SYSTEM_CASCADED_MENU,
  IDX_SYSTEM_ICON_IN_MENU,
  IDT_SYSTEM_CONTEXT_MENU_ITEMS
};

static LPCWSTR kSystemTopic = L"fm/options.htm#sevenZip";

struct CContextMenuItem
{
  int ControlID;
  UInt32 Flag;
};

static CContextMenuItem kMenuItems[] =
{
  { IDS_CONTEXT_OPEN, kOpen},
  { IDS_CONTEXT_OPEN, kOpenAs},
  { IDS_CONTEXT_EXTRACT, kExtract},
  { IDS_CONTEXT_EXTRACT_HERE, kExtractHere },
  { IDS_CONTEXT_EXTRACT_TO, kExtractTo },

  { IDS_CONTEXT_TEST, kTest},

  { IDS_CONTEXT_COMPRESS, kCompress },
  { IDS_CONTEXT_COMPRESS_TO, kCompressTo7z },
  { IDS_CONTEXT_COMPRESS_TO, kCompressToZip }

  #ifndef UNDER_CE
  ,
  { IDS_CONTEXT_COMPRESS_EMAIL, kCompressEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressTo7zEmail },
  { IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressToZipEmail }
  #endif

  , { IDS_PROP_CHECKSUM, kCRC }
};

bool CMenuPage::OnInit()
{
  _initMode = true;
  LangSetDlgItems(*this, kLangIDs, ARRAY_SIZE(kLangIDs));

  #ifdef UNDER_CE
  EnableItem(IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, false);
  #else
  CheckButton(IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, NZipRootRegistry::CheckContextMenuHandler());
  #endif

  CContextMenuInfo ci;
  ci.Load();

  CheckButton(IDX_SYSTEM_CASCADED_MENU, ci.Cascaded);
  CheckButton(IDX_SYSTEM_ICON_IN_MENU, ci.MenuIcons);

  _listView.Attach(GetItem(IDL_SYSTEM_OPTIONS));

  UInt32 newFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  _listView.SetExtendedListViewStyle(newFlags, newFlags);

  _listView.InsertColumn(0, L"", 100);

  for (int i = 0; i < ARRAY_SIZE(kMenuItems); i++)
  {
    CContextMenuItem &menuItem = kMenuItems[i];

    UString s = LangString(menuItem.ControlID);
    if (menuItem.Flag == kCRC)
      s = L"CRC SHA";
    if (menuItem.Flag == kOpenAs ||
        menuItem.Flag == kCRC)
      s += L" >";

    switch (menuItem.ControlID)
    {
      case IDS_CONTEXT_EXTRACT_TO:
      {
        s = MyFormatNew(s, LangString(IDS_CONTEXT_FOLDER));
        break;
      }
      case IDS_CONTEXT_COMPRESS_TO:
      case IDS_CONTEXT_COMPRESS_TO_EMAIL:
      {
        UString s2 = LangString(IDS_CONTEXT_ARCHIVE);
        switch (menuItem.Flag)
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
  if (IsButtonCheckedBool(IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU))
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
  ci.Cascaded = IsButtonCheckedBool(IDX_SYSTEM_CASCADED_MENU);
  ci.MenuIcons = IsButtonCheckedBool(IDX_SYSTEM_ICON_IN_MENU);
  ci.Flags = 0;
  for (int i = 0; i < ARRAY_SIZE(kMenuItems); i++)
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
  switch (buttonID)
  {
    case IDX_SYSTEM_INTEGRATE_TO_CONTEXT_MENU:
    case IDX_SYSTEM_CASCADED_MENU:
    case IDX_SYSTEM_ICON_IN_MENU:
      Changed();
      return true;
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);

}

bool CMenuPage::OnNotify(UINT controlID, LPNMHDR lParam)
{
  if (lParam->hwndFrom == HWND(_listView))
  {
    switch (lParam->code)
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
