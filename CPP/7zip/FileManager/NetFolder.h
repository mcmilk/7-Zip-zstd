// NetFolder.h

#ifndef __NETFOLDER_H
#define __NETFOLDER_H

#include "Common/String.h"
#include "Common/Buffer.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "Windows/Net.h"

#include "IFolder.h"

struct CResourceEx: public NWindows::NNet::CResourceW
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
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems);  
  STDMETHOD(GetProperty)(UInt32 itemIndex, PROPID propID, PROPVARIANT *value);
  STDMETHOD(BindToFolder)(UInt32 index, IFolderFolder **resultFolder);
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder);
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder);
  STDMETHOD(GetName)(BSTR *name);

  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);
  STDMETHOD(GetTypeID)(BSTR *name);
  STDMETHOD(GetPath)(BSTR *path);
  STDMETHOD(GetSystemIconIndex)(UInt32 index, INT32 *iconIndex);

private:
  NWindows::NNet::CResourceW _netResource;
  NWindows::NNet::CResourceW *_netResourcePointer;

  CObjectVector<CResourceEx> _items;

  CMyComPtr<IFolderFolder> _parentFolder;
  UString _path;
  
public:
  void Init(const UString &path);
  void Init(const NWindows::NNet::CResourceW *netResource, 
      IFolderFolder *parentFolder, const UString &path);
  CNetFolder(): _netResourcePointer(0) {}
};

#endif
