// ZipFolder.h

#pragma once

#ifndef __ZIPFOLDER_H
#define __ZIPFOLDER_H

#include "Common/String.h"
#include "../Common/IArchiveHandler2.h"
#include "Windows/ItemIDListUtils.h"
#include "ProxyHandler.h"

// {23170F69-40C1-278A-1000-000100010000}
DEFINE_GUID(CLSID_CZipFolder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00);

class CZipFolder: 
  public IShellFolder,
  public IPersistFolder,
  // public IShellChangeNotify,
  public CComObjectRoot,
  public CComCoClass<CZipFolder,&CLSID_CZipFolder>
{
public:

BEGIN_COM_MAP(CZipFolder)
  COM_INTERFACE_ENTRY(IShellFolder)
  COM_INTERFACE_ENTRY(IPersistFolder)
  // COM_INTERFACE_ENTRY(IShellChangeNotify)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CZipFolder)

  DECLARE_REGISTRY(CZipFolder, _T("SevenZip.ShellFolder.1"), _T("SevenZip.ShellFolder"), 0, THREADFLAGS_APARTMENT)

  STDMETHODIMP ParseDisplayName( HWND, LPBC p, LPOLESTR, ULONG FAR*,
      LPITEMIDLIST *, ULONG * );
  STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD aFlags, LPENUMIDLIST * anEnumIDList);
  STDMETHODIMP BindToObject( LPCITEMIDLIST, LPBC, REFIID, LPVOID FAR* );
  STDMETHODIMP BindToStorage( LPCITEMIDLIST, LPBC, REFIID, LPVOID FAR* );
  STDMETHODIMP CompareIDs( LPARAM, LPCITEMIDLIST, LPCITEMIDLIST );
  STDMETHODIMP CreateViewObject( HWND, REFIID, LPVOID FAR* );
  STDMETHODIMP GetAttributesOf( UINT, LPCITEMIDLIST FAR*, ULONG FAR* );
  STDMETHODIMP GetUIObjectOf( HWND, UINT, LPCITEMIDLIST FAR*, REFIID,
      UINT FAR*, LPVOID FAR* );
  STDMETHODIMP GetDisplayNameOf( LPCITEMIDLIST, DWORD, LPSTRRET );
  STDMETHODIMP SetNameOf( HWND, LPCITEMIDLIST, LPCOLESTR, DWORD,
      LPITEMIDLIST FAR* );
  
  // IPersistFolder methods
  STDMETHODIMP GetClassID(LPCLSID aClassID);
  STDMETHODIMP Initialize(LPCITEMIDLIST anIDList);
  
  /*
  // custom methods
  STDMETHODIMP GetFileName( LPTSTR szFileName, INT iBufSize );
  */

  // IShellChangeNotify methods
  // STDMETHODIMP OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

  CZipFolder();
  ~CZipFolder();

  void Init(CProxyHandler *aProxyHandlerSpec, 
      IArchiveHandler100 *anArchiveHandler,
      IArchiveFolder *anArchiveFolderItem, const UString &aFileName);
  HRESULT MyInitialize();
    
private:
  FOLDERSETTINGS m_FolderSettings;
  CComPtr<IMalloc> m_Malloc;
  CItemIDListManager *m_IDListManager;
  
  // CProxyHandler *m_ProxyHandler;
  // UINT64 m_HandlerCookie;

  // CComPtr<IShellFolder> m_ParentFolder;
  bool m_IsRootFolder;
  bool m_CantOpen;
public:
  NItemIDList::CHolder m_AbsoluteIDList;
  CComPtr<IArchiveFolder> m_ArchiveFolder;
  CComPtr<IArchiveHandler100> m_ArchiveHandler;
  CProxyHandler *m_ProxyHandlerSpec;
  CComPtr<IUnknown> m_ProxyHandler;
  UString m_FileName;
};

#endif