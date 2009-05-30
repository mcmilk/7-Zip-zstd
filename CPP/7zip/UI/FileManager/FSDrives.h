// FSDrives.h

#ifndef __FS_DRIVES_H
#define __FS_DRIVES_H

#include "Common/MyCom.h"
#include "Common/MyString.h"

#include "IFolder.h"

struct CDriveInfo
{
  UString Name;
  UString FullSystemName;
  bool KnownSizes;
  UInt64 DriveSize;
  UInt64 FreeSpace;
  UInt64 ClusterSize;
  // UString Type;
  UString VolumeName;
  UString FileSystemName;
  UINT DriveType;

  UString GetDeviceFileIoName() const;
};

class CFSDrives:
  public IFolderFolder,
  public IFolderOperations,
  public IFolderGetSystemIconIndex,
  public CMyUnknownImp
{
  CObjectVector<CDriveInfo> _drives;
  bool _volumeMode;

  HRESULT BindToFolderSpec(const wchar_t *name, IFolderFolder **resultFolder);
  UString GetExt(int index) const;
  HRESULT GetLength(int index, UInt64 &length) const;
public:
  MY_UNKNOWN_IMP2(IFolderGetSystemIconIndex, IFolderOperations)

  INTERFACE_FolderFolder(;)
  INTERFACE_FolderOperations(;)

  STDMETHOD(GetSystemIconIndex)(UInt32 index, Int32 *iconIndex);

  void Init(bool volMode = false)
  {
    _volumeMode = volMode;
  }
};

#endif
