// FSDrives.h

#ifndef __FSDRIVES_H
#define __FSDRIVES_H

#include "Common/String.h"
#include "Common/Types.h"
#include "Common/MyCom.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "IFolder.h"

struct CDriveInfo
{
  UString Name;
  UString FullSystemName;
  bool KnownSizes;
  UInt64 DriveSize;
  UInt64 FreeSpace;
  UInt64 ClusterSize;
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
  HRESULT BindToFolderSpec(const wchar_t *name, IFolderFolder **resultFolder);
  CObjectVector<CDriveInfo> _drives;
  bool _volumeMode;
public:
  void Init() { _volumeMode = false;}
};

#endif
