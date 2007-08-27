// FSDrives.h

#ifndef __FSDRIVES_H
#define __FSDRIVES_H

#include "Common/MyString.h"
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
  public IFolderGetSystemIconIndex,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(
    IFolderGetSystemIconIndex
  )

  INTERFACE_FolderFolder(;)

  STDMETHOD(GetSystemIconIndex)(UInt32 index, INT32 *iconIndex);

private:
  HRESULT BindToFolderSpec(const wchar_t *name, IFolderFolder **resultFolder);
  CObjectVector<CDriveInfo> _drives;
  bool _volumeMode;
public:
  void Init() { _volumeMode = false;}
};

#endif
