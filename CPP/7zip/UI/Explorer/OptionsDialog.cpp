// OptionsDialog.cpp

#include "StdAfx.h"

#include "resource.h"

#include "OptionsDialog.h"

#include "Common/StringConvert.h"
#include "Windows/Control/PropertyPage.h"

#include "../FileManager/LangUtils.h"
#include "FoldersPage.h"
#include "FoldersPageRes.h"
#include "SystemPage.h"
#include "SystemPageRes.h"

using namespace NWindows;

static INT_PTR OptionsDialog(HWND hwndOwner)
{
  CSystemPage systemPage;
  CFoldersPage foldersPage;
  UINT32 langIDs[] = { 0x01000300, 0x01000200};
  UINT pageIDs[] = { IDD_SYSTEM, IDD_FOLDERS};
  NControl::CPropertyPage *pagePinters[] = { &systemPage, &foldersPage };
  CObjectVector<NControl::CPageInfo> pages;
  const int kNumPages = sizeof(langIDs) / sizeof(langIDs[0]);
  for (int i = 0; i < kNumPages; i++)
  {
    NControl::CPageInfo page;
    page.Title = LangString(langIDs[i]);
    page.ID = pageIDs[i];
    page.Page = pagePinters[i];
    pages.Add(page);
  }
  return NControl::MyPropertySheet(pages, hwndOwner, 
    LangString(IDS_CONFIG_DIALOG_CAPTION, 0x01000000));
}

STDMETHODIMP CSevenZipOptions::PluginOptions(HWND hWnd, 
    IPluginOptionsCallback * /* callback */)
{
  /*
  CComBSTR programPath;
  RETUEN_IF_NOT_S_OK(callback->GetProgramPath(programName)));
  */
  OptionsDialog(hWnd);
  return S_OK;
}

STDMETHODIMP CSevenZipOptions::GetFileExtensions(BSTR * /* extensions */)
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


