// NetFolder.h

#ifndef __NETFOLDER_H
#define __NETFOLDER_H

#include "Common/String.h"
#include "Common/Buffer.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "Windows/Net.h"

#include "IFolder.h"

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
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP4(
    IEnumProperties,
    IFolderGetTypeID,
    IFolderGetPath,
    IFolderGetSystemIconIndex
  )

  STDMETHOD(LoadItems)();
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 itemIndex, PROPID propID, PROPVARIANT *value);
  STDMETHOD(BindToFolder)(UINT32 index, IFolderFolder **resultFolder);
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder);
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder);
  STDMETHOD(GetName)(BSTR *name);

  STDMETHOD(GetNumberOfProperties)(UINT32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);
  STDMETHOD(GetTypeID)(BSTR *name);
  STDMETHOD(GetPath)(BSTR *path);
  STDMETHOD(GetSystemIconIndex)(UINT32 index, INT32 *iconIndex);

private:
  NWindows::NNet::CResource _netResource;
  NWindows::NNet::CResource *_netResourcePointer;

  CObjectVector<CResourceEx> _items;

  CMyComPtr<IFolderFolder> _parentFolder;
  UString _path;
  
public:
  void Init(const UString &path);
  void Init(const NWindows::NNet::CResource *netResource, 
      IFolderFolder *parentFolder, const UString &path);
  CNetFolder(): _netResourcePointer(0) {}
};

#endif
