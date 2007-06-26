// RootFolder.h

#ifndef __ROOTFOLDER_H
#define __ROOTFOLDER_H

#include "Common/MyString.h"

#include "Windows/PropVariant.h"

#include "FSFolder.h"

class CRootFolder: 
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

  void Init();
private:
  UString _computerName;
  UString _networkName;
};

#endif
