// OptionsDialog.cpp

#include "StdAfx.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/PropertyPage.h"

#include "DialogSize.h"
#include "EditPage.h"
#include "EditPageRes.h"
#include "FoldersPage.h"
#include "FoldersPageRes.h"
#include "LangPage.h"
#include "LangPageRes.h"
#include "MenuPage.h"
#include "MenuPageRes.h"
// #include "PluginsPage.h"
// #include "PluginsPageRes.h"
#include "SettingsPage.h"
#include "SettingsPageRes.h"
#include "SystemPage.h"
#include "SystemPageRes.h"

#include "App.h"
#include "LangUtils.h"
#include "MyLoadMenu.h"
#include "ProgramLocation.h"

#include "resource.h"

using namespace NWindows;

#ifndef UNDER_CE
typedef UINT32 (WINAPI * DllRegisterServerPtr)();

extern HWND g_MenuPageHWND;

static void ShowMenuErrorMessage(const wchar_t *m)
{
  MessageBoxW(g_MenuPageHWND, m, L"7-Zip", MB_ICONERROR);
}

static int DllRegisterServer2(const char *name)
{
  NWindows::NDLL::CLibrary lib;

  UString prefix;
  GetProgramFolderPath(prefix);
  if (!lib.Load(prefix + L"7-zip.dll"))
  {
    ShowMenuErrorMessage(L"7-Zip cannot load 7-zip.dll");
    return E_FAIL;
  }
  DllRegisterServerPtr f = (DllRegisterServerPtr)lib.GetProc(name);
  if (f == NULL)
  {
    ShowMenuErrorMessage(L"Incorrect plugin");
    return E_FAIL;
  }
  HRESULT res = f();
  if (res != S_OK)
    ShowMenuErrorMessage(HResultToMessage(res));
  return (int)res;
}

STDAPI DllRegisterServer(void)
{
  #ifdef UNDER_CE
  return S_OK;
  #else
  return DllRegisterServer2("DllRegisterServer");
  #endif
}

STDAPI DllUnregisterServer(void)
{
  #ifdef UNDER_CE
  return S_OK;
  #else
  return DllRegisterServer2("DllUnregisterServer");
  #endif
}

#endif

void OptionsDialog(HWND hwndOwner, HINSTANCE /* hInstance */)
{
  CSystemPage systemPage;
  // CPluginsPage pluginsPage;
  CEditPage editPage;
  CSettingsPage settingsPage;
  CLangPage langPage;
  CMenuPage menuPage;
  CFoldersPage foldersPage;

  CObjectVector<NControl::CPageInfo> pages;
  UINT32 langIDs[] = { 0x03010300,
    // 0x03010100,
    0xFFFFFFFF,
    0x01000200, 0x03010200, 0x03010400, 0x01000400};
  
  BIG_DIALOG_SIZE(200, 200);

  UINT pageIDs[] = {
      SIZED_DIALOG(IDD_SYSTEM),
      // IDD_PLUGINS,
      SIZED_DIALOG(IDD_MENU),
      SIZED_DIALOG(IDD_FOLDERS),
      SIZED_DIALOG(IDD_EDIT),
      SIZED_DIALOG(IDD_SETTINGS),
      SIZED_DIALOG(IDD_LANG) };
  NControl::CPropertyPage *pagePinters[] = { &systemPage,  &menuPage, &foldersPage, &editPage, &settingsPage, &langPage };
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
    if (langPage.LangWasChanged)
    {
      g_App._window.SetText(LangString(IDS_APP_TITLE, 0x03000000));
      MyLoadMenu();
      g_App.ReloadToolbars();
      g_App.MoveSubWindows();
    }
    /*
    if (systemPage.WasChanged)
    {
      // probably it doesn't work, since image list is locked?
      g_App.SysIconsWereChanged();
    }
    */
    g_App.SetListSettings();
    g_App.SetShowSystemMenu();
    g_App.RefreshAllPanels();
    // ::PostMessage(hwndOwner, kLangWasChangedMessage, 0 , 0);
  }
}
