// PhysDriveFolder.h

#ifndef __PHYSDRIVEFOLDER_H 
#define __PHYSDRIVEFOLDER_H 

#include "Common/MyString.h"
#include "Common/MyCom.h"

#include "IFolder.h"

class CPhysDriveFolder: 
  public IFolderFolder,
  public IFolderWasChanged,
  public IFolderOperations,
  public IFolderGetItemFullSize,
  public IFolderClone,
  // public IFolderGetSystemIconIndex,
  public CMyUnknownImp
{
  UInt64 GetSizeOfItem(int anIndex) const;
public:
  MY_QUERYINTERFACE_BEGIN
    MY_QUERYINTERFACE_ENTRY(IFolderWasChanged)
    MY_QUERYINTERFACE_ENTRY(IFolderOperations)
    MY_QUERYINTERFACE_ENTRY(IFolderGetItemFullSize)
    MY_QUERYINTERFACE_ENTRY(IFolderClone)
    // MY_QUERYINTERFACE_ENTRY(IFolderGetSystemIconIndex)
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE


  INTERFACE_FolderFolder(;)
  INTERFACE_FolderOperations(;)

  STDMETHOD(WasChanged)(INT32 *wasChanged);
  STDMETHOD(Clone)(IFolderFolder **resultFolder);
  STDMETHOD(GetItemFullSize)(UInt32 index, PROPVARIANT *value, IProgress *progress);

  // STDMETHOD(GetSystemIconIndex)(UInt32 index, INT32 *iconIndex);

private:
  UString _name;
  UString _prefix;
  UString _path;
  UString GetFullPath() const { return _prefix + _path; }
  UString GetFullPathWithName() const { return GetFullPath() + L'\\' + _name; }
  CMyComPtr<IFolderFolder> _parentFolder;

  UINT _driveType;
  DISK_GEOMETRY geom;
  UInt64 _length;

public:
  HRESULT Init(const UString &path);
  HRESULT GetLength(UInt64 &size) const;
};

#endif
