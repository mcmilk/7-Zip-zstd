// OptionsDialog.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/StringConvert.h"

#include "Windows/Control/PropertyPage.h"
#include "Windows/Error.h"

#include "Resource/LangPage/LangPage.h"
#include "Resource/LangPage/resource.h"
#include "Resource/PluginsPage/PluginsPage.h"
#include "Resource/PluginsPage/resource.h"
#include "Resource/SystemPage/SystemPage.h"
#include "Resource/SystemPage/resource.h"
#include "Resource/EditPage/EditPage.h"
#include "Resource/EditPage/resource.h"
#include "Resource/SettingsPage/SettingsPage.h"
#include "Resource/SettingsPage/resource.h"

#include "LangUtils.h"
#include "MyLoadMenu.h"
#include "App.h"

void FillInPropertyPage(PROPSHEETPAGE* page, HINSTANCE instance, int dialogID, 
    NWindows::NControl::CPropertyPage *propertyPage, const CSysString &title)
{
  page->dwSize = sizeof(PROPSHEETPAGE);
  // page->dwSize = sizeof(PROPSHEETPAGEW_V1_SIZE);

  page->dwFlags = PSP_HASHELP;
  page->hInstance = instance;
  page->pszTemplate = MAKEINTRESOURCE(dialogID);
  page->pszIcon = NULL;
  page->pfnDlgProc = NWindows::NControl::ProperyPageProcedure;

  if (title.IsEmpty())
    page->pszTitle = NULL;
  else
  {
    page->dwFlags |= PSP_USETITLE;
    page->pszTitle = title;
  }
 
  page->lParam = LPARAM(propertyPage);
  page->pfnCallback = NULL;

  // page->dwFlags = 0;
  // page->pszTitle = NULL;
  // page->lParam = 0;
}

void OptionsDialog(HWND hwndOwner, HINSTANCE hInstance)
{
  CSystemPage systemPage;
  CPluginsPage pluginsPage;
  CEditPage editPage;
  CSettingsPage settingsPage;
  CLangPage langPage;

  CSysStringVector titles;
  UINT32 langIDs[] = { 0x03010300, 0x03010100, 0x03010200, 0x03010400, 0x01000400};
  const int kNumPages = sizeof(langIDs) / sizeof(langIDs[0]);
  for (int i = 0; i < kNumPages; i++)
    titles.Add(GetSystemString(LangLoadString(langIDs[i])));
  
  PROPSHEETPAGE pages[kNumPages];

  FillInPropertyPage(&pages[0], hInstance, IDD_SYSTEM, &systemPage, titles[0]);
  FillInPropertyPage(&pages[1], hInstance, IDD_PLUGINS, &pluginsPage, titles[1]);
  FillInPropertyPage(&pages[2], hInstance, IDD_EDIT, &editPage, titles[2]);
  FillInPropertyPage(&pages[3], hInstance, IDD_SETTINGS, &settingsPage, titles[3]);
  FillInPropertyPage(&pages[4], hInstance, IDD_LANG, &langPage, titles[4]);

  PROPSHEETHEADER sheet;

  // sheet.dwSize = sizeof(PROPSHEETHEADER_V1_SIZE);

  sheet.dwSize = sizeof(PROPSHEETHEADER);
  sheet.dwFlags = PSH_PROPSHEETPAGE;
  sheet.hwndParent = hwndOwner;
  sheet.hInstance = hInstance;

  CSysString title = LangLoadString(IDS_OPTIONS, 0x03010000);

  sheet.pszCaption = title;
  sheet.nPages = sizeof(pages) / sizeof(PROPSHEETPAGE);
  sheet.nStartPage = 0;
  sheet.ppsp = pages;
  sheet.pfnCallback = NULL;

  if (::PropertySheet(&sheet) != -1)
  {
    if (langPage._langWasChanged)
      MyLoadMenu();
    g_App.SetListSettings();
    g_App.SetShowSystemMenu();
    g_App.RefreshAllPanels();
      // ::PostMessage(hwndOwner, kLangWasChangedMessage, 0 , 0);
  }
  /*
  else
    MessageBox(0, NWindows::NError::MyFormatMessage(GetLastError()), TEXT("PropertySheet"), 0);
  */

}

