// ZipEnum.h

#pragma once

#ifndef __ZIPENUM_H
#define __ZIPENUM_H

#include "../Common/IArchiveHandler2.h"
#include "Windows/ItemIDListUtils.h"

class CZipFolder;

class CZipEnumIDList: 
  public IEnumIDList,
  public CComObjectRoot
{
public:
  CZipEnumIDList():m_Index(0) {}; 

BEGIN_COM_MAP(CZipEnumIDList)
  COM_INTERFACE_ENTRY(IEnumIDList)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CZipEnumIDList)

DECLARE_NO_REGISTRY()

	// IEnumIDList methods 
	STDMETHODIMP Next( ULONG, LPITEMIDLIST *, ULONG * );
  STDMETHODIMP Skip( ULONG );
  STDMETHODIMP Reset();
  STDMETHODIMP Clone( IEnumIDList ** );

  void Init(DWORD aFlags, IArchiveFolder *anArchiveFolderItem);

private:
  CItemIDListManager m_IDListManager;
  CZipFolder *m_Folder;
  CComPtr<IShellFolder> m_FolderRef;
  DWORD     m_Flags;
  
  int m_Index;

  UINT32 m_NumSubItems;
  // CProxyHandler *m_ProxyHandler;
  CComPtr<IArchiveFolder> m_ArchiveFolder;
  
  bool TestFileItem(/*const CArchiveFolderFileItem &anItem*/);
  bool TestDirItem(/*const CArchiveFolderItem &anItem*/);
};

#endif