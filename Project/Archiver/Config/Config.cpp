// ZipConfig2.cpp : Defines the entry point for the application.
//

#include "StdAfx.h"

#include "resource.h"

#include "SystemPage.h"
#include "InfoPage.h"
#include "FoldersPage.h"

int CreatePropertySheet(HWND hwndOwner, HINSTANCE hInst);


HINSTANCE g_hInstance;

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  // Perform initializations that apply to a specific instance 
  g_hInstance = (HINSTANCE)hInstance;
  CreatePropertySheet(NULL, (HINSTANCE)hInstance);
  return 0;
}


void FillInPropertyPage(PROPSHEETPAGE* aPage, 
    HINSTANCE anInstance, 
    int aDialogID, 
    NWindows::NControl::CPropertyPage *aPropertyPage)
{
  aPage->dwSize = sizeof(PROPSHEETPAGE);
  aPage->dwFlags = PSP_HASHELP;
  aPage->hInstance = anInstance;
  aPage->pszTemplate = MAKEINTRESOURCE(aDialogID);
  aPage->pszIcon = NULL;
  aPage->pfnDlgProc = NWindows::NControl::ProperyPageProcedure;
  aPage->pszTitle = NULL;
  aPage->lParam = LPARAM(aPropertyPage);
}

int CreatePropertySheet(HWND hwndOwner, HINSTANCE hInstance)
{
  const kNumPages = 3;

  PROPSHEETPAGE aPages[kNumPages];
  
  CSystemPage aSystemPage;
  CFoldersPage aFoldersPage;
  CInfoPage anInfoPage;

  FillInPropertyPage(&aPages[0], hInstance, IDD_SYSTEM, &aSystemPage);
  FillInPropertyPage(&aPages[1], hInstance, IDD_FOLDERS, &aFoldersPage);
  FillInPropertyPage(&aPages[2], hInstance, IDD_INFO, &anInfoPage);

  PROPSHEETHEADER aSheet;

  aSheet.dwSize = sizeof(PROPSHEETHEADER);
  // aSheet.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
  aSheet.dwFlags = PSH_PROPSHEETPAGE;
  aSheet.hwndParent = hwndOwner;
  aSheet.hInstance = hInstance;
  aSheet.pszCaption = TEXT("7-Zip Configuration");
  aSheet.nPages = sizeof(aPages) / sizeof(PROPSHEETPAGE);
  aSheet.nStartPage = 0;
  aSheet.ppsp = aPages;
  
  return (PropertySheet(&aSheet));
}
