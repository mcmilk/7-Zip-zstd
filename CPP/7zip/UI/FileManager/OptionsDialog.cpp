// OptionsDialog.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/StringConvert.h"

#include "Windows/Control/PropertyPage.h"
#include "Windows/Error.h"

#include "LangPage.h"
#include "LangPageRes.h"
#include "PluginsPage.h"
#include "PluginsPageRes.h"
#include "SystemPage.h"
#include "SystemPageRes.h"
#include "EditPage.h"
#include "EditPageRes.h"
#include "SettingsPage.h"
#include "SettingsPageRes.h"

#include "LangUtils.h"
#include "MyLoadMenu.h"
#include "App.h"

using namespace NWindows;

void OptionsDialog(HWND hwndOwner, HINSTANCE /* hInstance */)
{
  CSystemPage systemPage;
  CPluginsPage pluginsPage;
  CEditPage editPage;
  CSettingsPage settingsPage;
  CLangPage langPage;

  CObjectVector<NControl::CPageInfo> pages;
  UINT32 langIDs[] = { 0x03010300, 0x03010100, 0x03010200, 0x03010400, 0x01000400};
  UINT pageIDs[] = { IDD_SYSTEM, IDD_PLUGINS, IDD_EDIT, IDD_SETTINGS, IDD_LANG};
  NControl::CPropertyPage *pagePinters[] = { &systemPage, &pluginsPage, &editPage, &settingsPage, &langPage };
  const int kNumPages = sizeof(langIDs) / sizeof(langIDs[0]);
  for (int i = 0; i < kNumPages; i++)
  {
    NControl::CPageInfo page;
    page.Title = LangString(langIDs[i]);
    page.ID = pageIDs[i];
    page.Page = pagePinters[i];
    pages.Add(page);
  }

  INT_PTR res = NControl::MyPropertySheet(pages, hwndOwner, LangString(IDS_OPTIONS, 0x03010000));
  if (res != -1 && res != 0)
  {
    if (langPage._langWasChanged)
    {
      g_App._window.SetText(LangString(IDS_APP_TITLE, 0x03000000));
      MyLoadMenu();
    }
    g_App.SetListSettings();
    g_App.SetShowSystemMenu();
    g_App.RefreshAllPanels();
    g_App.ReloadToolbars();
    // ::PostMessage(hwndOwner, kLangWasChangedMessage, 0 , 0);
  }
}
