// IFolder.h

#ifndef __IFOLDER_H
#define __IFOLDER_H

#include "../../IProgress.h"

#define FOLDER_INTERFACE_SUB(i, b, x) DECL_INTERFACE_SUB(i, b, 8, x)
#define FOLDER_INTERFACE(i, x) FOLDER_INTERFACE_SUB(i, IUnknown, x)

namespace NPlugin
{
  enum 
  {
    kName = 0,
    kType,
    kClassID,
    kOptionsClassID
  };
}

#define INTERFACE_FolderFolder(x) \
  STDMETHOD(LoadItems)() x; \
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems) x; \
  STDMETHOD(GetProperty)(UInt32 itemIndex, PROPID propID, PROPVARIANT *value) x; \
  STDMETHOD(BindToFolder)(UInt32 index, IFolderFolder **resultFolder) x; \
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder) x; \
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder) x; \
  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties) x; \
  STDMETHOD(GetPropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType) x; \
  STDMETHOD(GetFolderProperty)(PROPID propID, PROPVARIANT *value) x; \

FOLDER_INTERFACE(IFolderFolder, 0x00)
{
  INTERFACE_FolderFolder(PURE)
};

FOLDER_INTERFACE(IFolderWasChanged, 0x04)
{
  STDMETHOD(WasChanged)(Int32 *wasChanged) PURE;
};

FOLDER_INTERFACE_SUB(IFolderOperationsExtractCallback, IProgress, 0x0B)
{
  // STDMETHOD(SetTotalFiles)(UInt64 total) PURE;
  // STDMETHOD(SetCompletedFiles)(const UInt64 *completedValue) PURE;
  STDMETHOD(AskWrite)(
      const wchar_t *srcPath, 
      Int32 srcIsFolder, 
      const FILETIME *srcTime, 
      const UInt64 *srcSize,
      const wchar_t *destPathRequest, 
      BSTR *destPathResult, 
      Int32 *writeAnswer) PURE;
  STDMETHOD(ShowMessage)(const wchar_t *message) PURE;
  STDMETHOD(SetCurrentFilePath)(const wchar_t *filePath) PURE;
  STDMETHOD(SetNumFiles)(UInt64 numFiles) PURE;
};

#define INTERFACE_FolderOperations(x) \
  STDMETHOD(CreateFolder)(const wchar_t *name, IProgress *progress) x; \
  STDMETHOD(CreateFile)(const wchar_t *name, IProgress *progress) x; \
  STDMETHOD(Rename)(UInt32 index, const wchar_t *newName, IProgress *progress) x; \
  STDMETHOD(Delete)(const UInt32 *indices, UInt32 numItems, IProgress *progress) x; \
  STDMETHOD(CopyTo)(const UInt32 *indices, UInt32 numItems, \
      const wchar_t *path, IFolderOperationsExtractCallback *callback) x; \
  STDMETHOD(MoveTo)(const UInt32 *indices, UInt32 numItems, \
      const wchar_t *path, IFolderOperationsExtractCallback *callback) x; \
  STDMETHOD(CopyFrom)(const wchar_t *fromFolderPath, \
      const wchar_t **itemsPaths, UInt32 numItems, IProgress *progress) x; \
  STDMETHOD(SetProperty)(UInt32 index, PROPID propID, const PROPVARIANT *value, IProgress *progress) x; \

FOLDER_INTERFACE(IFolderOperations, 0x06)
{
  INTERFACE_FolderOperations(PURE)
};

/*
FOLDER_INTERFACE2(IFolderOperationsDeleteToRecycleBin, 0x06, 0x03)
{
  STDMETHOD(DeleteToRecycleBin)(const UInt32 *indices, UInt32 numItems, IProgress *progress) PURE;
};
*/

FOLDER_INTERFACE(IFolderGetSystemIconIndex, 0x07)
{
  STDMETHOD(GetSystemIconIndex)(UInt32 index, Int32 *iconIndex) PURE;
};

FOLDER_INTERFACE(IFolderGetItemFullSize, 0x08)
{
  STDMETHOD(GetItemFullSize)(UInt32 index, PROPVARIANT *value, IProgress *progress) PURE;
};

FOLDER_INTERFACE(IFolderClone, 0x09)
{
  STDMETHOD(Clone)(IFolderFolder **resultFolder) PURE;
};

FOLDER_INTERFACE(IFolderSetFlatMode, 0x0A)
{
  STDMETHOD(SetFlatMode)(Int32 flatMode) PURE;
};

#define INTERFACE_FolderProperties(x) \
  STDMETHOD(GetNumberOfFolderProperties)(UInt32 *numProperties) x; \
  STDMETHOD(GetFolderPropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType) x; \

FOLDER_INTERFACE(IFolderProperties, 0x0B)
{
  INTERFACE_FolderProperties(PURE)
};

#define INTERFACE_IFolderArchiveProperties(x) \
  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value) x; \
  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties) x; \
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType) x;

FOLDER_INTERFACE(IFolderArchiveProperties, 0x0C)
{
  INTERFACE_IFolderArchiveProperties(PURE)
};

FOLDER_INTERFACE(IGetFolderArchiveProperties, 0x0D)
{
  STDMETHOD(GetFolderArchiveProperties)(IFolderArchiveProperties **object) PURE;
};

#define FOLDER_MANAGER_INTERFACE(i, x)  DECL_INTERFACE(i, 9, x)

#define INTERFACE_IFolderManager(x) \
  STDMETHOD(OpenFolderFile)(const wchar_t *filePath, IFolderFolder **resultFolder, IProgress *progress) x; \
  STDMETHOD(GetExtensions)(BSTR *extensions) x; \
  STDMETHOD(GetIconPath)(const wchar_t *ext, BSTR *iconPath, Int32 *iconIndex) x; \
  
  // STDMETHOD(GetTypes)(BSTR *types) PURE;
  // STDMETHOD(CreateFolderFile)(const wchar_t *type, const wchar_t *filePath, IProgress *progress) PURE;
            
FOLDER_MANAGER_INTERFACE(IFolderManager, 0x03)
{
  INTERFACE_IFolderManager(PURE);
};


#endif
