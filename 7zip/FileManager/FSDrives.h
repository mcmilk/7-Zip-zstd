// FSDrives.h

#pragma once

#ifndef __FSDRIVES_H
#define __FSDRIVES_H

#include "Common/String.h"
#include "Common/MyCom.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "IFolder.h"

struct CDriveInfo
{
  UString Name;
  UString FullSystemName;
  bool KnownSizes;
  UINT64 DriveSize;
  UINT64 FreeSpace;
  UINT64 ClusterSize;
  UString Type;
  UString VolumeName;
  UString FileSystemName;
};

class CFSDrives: 
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
  HRESULT BindToFolderSpec(const wchar_t *name, IFolderFolder **resultFolder);
  CObjectVector<CDriveInfo> _drives;
public:
  void Init() {}
};

#endif
