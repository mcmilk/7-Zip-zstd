// NetFolder.h

#pragma once

#ifndef __NETFOLDER_H
#define __NETFOLDER_H

#include "Common/String.h"
#include "Common/Buffer.h"

#include "Windows/PropVariant.h"
#include "Windows/Net.h"

#include "FolderInterface.h"

struct CResourceEx: public NWindows::NNet::CResource
{
  UString Name;
};

class CNetFolder: 
  public IFolderFolder,
  public IEnumProperties,
  public IFolderGetTypeID,
  public IFolderGetPath,
  public IFolderGetSystemIconIndex,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CNetFolder)
  COM_INTERFACE_ENTRY(IFolderFolder)
  COM_INTERFACE_ENTRY(IEnumProperties)
  COM_INTERFACE_ENTRY(IFolderGetTypeID)
  COM_INTERFACE_ENTRY(IFolderGetPath)
  COM_INTERFACE_ENTRY(IFolderGetSystemIconIndex)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CNetFolder)

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

private:
  NWindows::NNet::CResource _netResource;
  NWindows::NNet::CResource *_netResourcePointer;

  CObjectVector<CResourceEx> _items;

  CComPtr<IFolderFolder> _parentFolder;
  UString _path;
  
public:
  void Init(const UString &path);
  void Init(const NWindows::NNet::CResource *netResource, 
      IFolderFolder *parentFolder, const UString &path);
  CNetFolder(): _netResourcePointer(0) {}
};

#endif
