// RootFolder.h

#pragma once

#ifndef __ROOTFOLDER_H
#define __ROOTFOLDER_H

#include "Common/String.h"

#include "Windows/PropVariant.h"

#include "FSFolder.h"

class CRootFolder: 
  public IFolderFolder,
  public IEnumProperties,
  public IFolderGetTypeID,
  public IFolderGetPath,
  public IFolderGetSystemIconIndex,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CRootFolder)
  COM_INTERFACE_ENTRY(IFolderFolder)
  COM_INTERFACE_ENTRY(IEnumProperties)
  COM_INTERFACE_ENTRY(IFolderGetTypeID)
  COM_INTERFACE_ENTRY(IFolderGetPath)
  COM_INTERFACE_ENTRY(IFolderGetSystemIconIndex)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CRootFolder)

DECLARE_NO_REGISTRY()

  STDMETHOD(LoadItems)();
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 itemIndex, PROPID propID, PROPVARIANT *value);
  STDMETHOD(BindToFolder)(UINT32 index, IFolderFolder **resultFolder);
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder);
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder);
  STDMETHOD(GetName)(BSTR *name);

  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetTypeID)(BSTR *name);
  STDMETHOD(GetPath)(BSTR *path);
  STDMETHOD(GetSystemIconIndex)(UINT32 index, INT32 *iconIndex);

  void Init();
private:
  UString _computerName;
  UString _networkName;
};

#endif
