// PhysDriveFolder.h

#ifndef __PHYSDRIVEFOLDER_H 
#define __PHYSDRIVEFOLDER_H 

#include "Common/MyString.h"
#include "Common/MyCom.h"

#include "IFolder.h"

class CPhysDriveFolder: 
  public IFolderFolder,
  public IEnumProperties,
  public IFolderGetTypeID,
  public IFolderGetPath,
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
    MY_QUERYINTERFACE_ENTRY(IEnumProperties)
    MY_QUERYINTERFACE_ENTRY(IFolderGetTypeID)
    MY_QUERYINTERFACE_ENTRY(IFolderGetPath)
    MY_QUERYINTERFACE_ENTRY(IFolderWasChanged)
    MY_QUERYINTERFACE_ENTRY(IFolderOperations)
    MY_QUERYINTERFACE_ENTRY(IFolderGetItemFullSize)
    MY_QUERYINTERFACE_ENTRY(IFolderClone)
    // MY_QUERYINTERFACE_ENTRY(IFolderGetSystemIconIndex)
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE


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
  STDMETHOD(WasChanged)(INT32 *wasChanged);
  STDMETHOD(Clone)(IFolderFolder **resultFolder);
  STDMETHOD(GetItemFullSize)(UInt32 index, PROPVARIANT *value, IProgress *progress);

  // IFolderOperations
  STDMETHOD(CreateFolder)(const wchar_t *name, IProgress *progress);
  STDMETHOD(CreateFile)(const wchar_t *name, IProgress *progress);
  STDMETHOD(Rename)(UInt32 index, const wchar_t *newName, IProgress *progress);
  STDMETHOD(Delete)(const UInt32 *indices, UInt32 numItems, IProgress *progress);
  STDMETHOD(CopyTo)(const UInt32 *indices, UInt32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback);
  STDMETHOD(MoveTo)(const UInt32 *indices, UInt32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback);
  STDMETHOD(CopyFrom)(const wchar_t *fromFolderPath,
      const wchar_t **itemsPaths, UInt32 numItems, IProgress *progress);
  STDMETHOD(SetProperty)(UInt32 index, PROPID propID, const PROPVARIANT *value, IProgress *progress);
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
