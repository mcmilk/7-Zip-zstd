// ZipConfig2.cpp : Defines the entry point for the application.
//

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "resource.h"

#include "SystemPage.h"
#include "InfoPage.h"
#include "FoldersPage.h"
#include "LangPage.h"

#include "../Common/LangUtils.h"

int CreatePropertySheet(HWND hwndOwner, HINSTANCE hInst);

HINSTANCE g_hInstance;

static bool IsItWindowsNT()
{
  OSVERSIONINFO aVersionInfo;
  aVersionInfo.dwOSVersionInfoSize = sizeof(aVersionInfo);
  if (!::GetVersionEx(&aVersionInfo)) 
    return false;
  return (aVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  #ifdef UNICODE
  if (!IsItWindowsNT())
  {
    ::MessageBox(0, TEXT("This program requires Windows NT/2000/XP"), 
        TEXT("7-Zip Configuration"), 0);
    return 0;
  }
  #endif
  // Perform initializations that apply to a specific instance 
  g_hInstance = (HINSTANCE)hInstance;

  CreatePropertySheet(NULL, (HINSTANCE)hInstance);
  return 0;
}


void FillInPropertyPage(PROPSHEETPAGE* aPage, 
    HINSTANCE anInstance, 
    int aDialogID, 
    NWindows::NControl::CPropertyPage *aPropertyPage,
    CSysString &aTitle)
{
  aPage->dwSize = sizeof(PROPSHEETPAGE);
  aPage->dwFlags = PSP_HASHELP;
  aPage->hInstance = anInstance;
  aPage->pszTemplate = MAKEINTRESOURCE(aDialogID);
  aPage->pszIcon = NULL;
  aPage->pfnDlgProc = NWindows::NControl::ProperyPageProcedure;

  if (aTitle.IsEmpty())
    aPage->pszTitle = NULL;
  else
  {
    aPage->dwFlags |= PSP_USETITLE;
    aPage->pszTitle = aTitle;
  }
  aPage->lParam = LPARAM(aPropertyPage);
}

int CreatePropertySheet(HWND hwndOwner, HINSTANCE hInstance)
{
  const kNumPages = 4;

  PROPSHEETPAGE aPages[kNumPages];
  
  CSystemPage aSystemPage;
  CFoldersPage aFoldersPage;
  CLangPage aLangPage;
  CInfoPage anInfoPage;

  CSysStringVector aTitles;
  UINT32 aLangIDs[] = { 0x01000300, 0x01000200, 0x01000400, 0x01000100 };
  for (int i = 0; i < sizeof(aLangIDs) / sizeof(aLangIDs[0]); i++)
    aTitles.Add(LangLoadString(aLangIDs[i]));

  FillInPropertyPage(&aPages[0], hInstance, IDD_SYSTEM, &aSystemPage, aTitles[0]);
  FillInPropertyPage(&aPages[1], hInstance, IDD_FOLDERS, &aFoldersPage, aTitles[1]);
  FillInPropertyPage(&aPages[2], hInstance, IDD_LANG, &aLangPage, aTitles[2]);
  FillInPropertyPage(&aPages[3], hInstance, IDD_INFO, &anInfoPage, aTitles[3]);

  PROPSHEETHEADER aSheet;

  aSheet.dwSize = sizeof(PROPSHEETHEADER);
  // aSheet.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
  aSheet.dwFlags = PSH_PROPSHEETPAGE;
  aSheet.hwndParent = hwndOwner;
  aSheet.hInstance = hInstance;

  CSysString aTitle = LangLoadString(IDS_CONFIG_DIALOG_CAPTION, 0x01000000);
  aSheet.pszCaption = aTitle;
  aSheet.nPages = sizeof(aPages) / sizeof(PROPSHEETPAGE);
  aSheet.nStartPage = 0;
  aSheet.ppsp = aPages;
  
  return (PropertySheet(&aSheet));
}
