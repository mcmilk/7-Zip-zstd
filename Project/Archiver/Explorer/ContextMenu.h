// ContextMenu.h

#pragma once

#ifndef __CONTEXTMENU_H
#define __CONTEXTMENU_H

// {23170F69-40C1-278A-1000-000100020000}
DEFINE_GUID(CLSID_CZipContextMenu, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);

#include "Common/String.h"

class CZipContextMenu: 
  public IContextMenu,
  public IShellExtInit,
  public CComObjectRoot,
  public CComCoClass<CZipContextMenu, &CLSID_CZipContextMenu>
{
  enum ECommandInternalID
  {
    kCommandInternalNULL,
    kCommandInternalIDOpen,
    kCommandInternalIDExtract,
    kCommandInternalIDCompress,
    kCommandInternalIDTest
  };
  
  struct CCommandMapItem
  {
    ECommandInternalID CommandInternalID;
    CSysString Verb;
    CSysString HelpString;
  };


public:

BEGIN_COM_MAP(CZipContextMenu)
  COM_INTERFACE_ENTRY(IContextMenu)
  COM_INTERFACE_ENTRY(IShellExtInit)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CZipContextMenu)

DECLARE_REGISTRY(CZipContextMenu, _T("SevenZip.ContextMenu.1"), _T("SevenZip.ContextMenu"), 0, THREADFLAGS_APARTMENT)

  ///////////////////////////////
  // IShellExtInit

  STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, 
      LPDATAOBJECT aDataObject, HKEY hkeyProgID);

  /////////////////////////////
  // IContextMenu
  
  STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu,
      UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
  STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
  STDMETHOD(GetCommandString)(UINT idCmd, UINT uType, UINT *pwReserved,
      LPSTR pszName, UINT cchMax);
private:
  CSysStringVector m_FileNames;
  std::vector<CCommandMapItem> m_CommandMap;
  HRESULT GetFileNames(LPDATAOBJECT aDataObject, CSysStringVector &aFileNames);
  UINT FindVerb(const CSysString &aVerb);
  STDMETHODIMP OpenArchive(CSysString &FileName);
};

#endif