// OptionsDialog.cpp

#include "StdAfx.h"

#include "resource.h"

#include "OptionsDialog.h"

#include "Windows/Control/PropertyPage.h"

#include "../../FileManager/LangUtils.h"
#include "../Resource/FoldersPage/FoldersPage.h"
#include "../Resource/FoldersPage/resource.h"
#include "../Resource/SystemPage/SystemPage.h"
#include "../Resource/SystemPage/resource.h"

extern HINSTANCE g_hInstance;

static void FillInPropertyPage(PROPSHEETPAGE* page, 
    HINSTANCE instance, 
    int dialogID, 
    NWindows::NControl::CPropertyPage *propertyPage,
    CSysString &title)
{
  page->dwSize = sizeof(PROPSHEETPAGE);
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
}

int OptionsDialog(HWND hwndOwner, HINSTANCE hInstance)
{
  const kNumPages = 2;

  PROPSHEETPAGE pages[kNumPages];
  
  CSystemPage systemPage;
  CFoldersPage foldersPage;
  
  CSysStringVector titles;
  UINT32 langIDs[] = { 0x01000300, 0x01000200};
  for (int i = 0; i < sizeof(langIDs) / sizeof(langIDs[0]); i++)
    titles.Add(LangLoadString(langIDs[i]));

  FillInPropertyPage(&pages[0], hInstance, IDD_SYSTEM, &systemPage, titles[0]);
  FillInPropertyPage(&pages[1], hInstance, IDD_FOLDERS, &foldersPage, titles[1]);

  PROPSHEETHEADER sheet;

  sheet.dwSize = sizeof(PROPSHEETHEADER);
  sheet.dwFlags = PSH_PROPSHEETPAGE;
  sheet.hwndParent = hwndOwner;
  sheet.hInstance = hInstance;

  CSysString title = LangLoadString(IDS_CONFIG_DIALOG_CAPTION, 0x01000000);

  sheet.pszCaption = title;
  sheet.nPages = sizeof(pages) / sizeof(PROPSHEETPAGE);
  sheet.nStartPage = 0;
  sheet.ppsp = pages;
  
  return (PropertySheet(&sheet));
}

STDMETHODIMP CSevenZipOptions::PluginOptions(HWND hWnd, 
    IPluginOptionsCallback *callback)
{
  /*
  CComBSTR programPath;
  RETUEN_IF_NOT_S_OK(callback->GetProgramPath(programName)));
  */
  OptionsDialog(hWnd, g_hInstance);
  return S_OK;
}

STDMETHODIMP CSevenZipOptions::GetFileExtensions(BSTR *extensions)
{
  /*
  UString extStrings;
  CObjectVector<NZipRootRegistry::CArchiverInfo> formats;
  NZipRootRegistry::ReadArchiverInfoList(formats);
  for(int i = 0; i < formats.Size(); i++)
  {
    if (i != 0)
      extStrings += L' ';
    extStrings += formats[i].Extension;
  }
  CComBSTR valueTemp = extStrings;
  *extensions = valueTemp.Detach();
  return S_OK;
  */
  return E_NOTIMPL;
}


