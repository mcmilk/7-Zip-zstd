// FSDrives.h

#pragma once

#ifndef __FSDRIVES_H
#define __FSDRIVES_H

#include "Common/String.h"

#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "FolderInterface.h"

struct CDriveInfo
{
  UString Name;
  CSysString FullSystemName;
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
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CFSDrives)
  COM_INTERFACE_ENTRY(IFolderFolder)
  COM_INTERFACE_ENTRY(IEnumProperties)
  COM_INTERFACE_ENTRY(IFolderGetTypeID)
  COM_INTERFACE_ENTRY(IFolderGetPath)
  COM_INTERFACE_ENTRY(IFolderGetSystemIconIndex)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CFSDrives)

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
  HRESULT BindToFolderSpec(const TCHAR *name, IFolderFolder **resultFolder);
  CObjectVector<CDriveInfo> _drives;
public:
  void Init() {}
};

#endif
