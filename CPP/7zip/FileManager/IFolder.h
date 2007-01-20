// FolderInterface.h

#ifndef __FOLDERINTERFACE_H
#define __FOLDERINTERFACE_H

#include "../IProgress.h"

#define FOLDER_INTERFACE_SUB(i, b, x, y) \
DEFINE_GUID(IID_ ## i, \
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, x, y, 0x00); \
struct i: public b

#define FOLDER_INTERFACE2(i, x, y) FOLDER_INTERFACE_SUB(i, IUnknown, x, y)

#define FOLDER_INTERFACE(i, x) FOLDER_INTERFACE2(i, x, 0x00)

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

FOLDER_INTERFACE(IFolderFolder, 0x00)
{
  STDMETHOD(LoadItems)() PURE;
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems) PURE;  
  // STDMETHOD(GetNumberOfSubFolders)(UInt32 *numSubFolders) PURE;  
  STDMETHOD(GetProperty)(UInt32 itemIndex, PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(BindToFolder)(UInt32 index, IFolderFolder **resultFolder) PURE;
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder) PURE;
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder) PURE;
  STDMETHOD(GetName)(BSTR *name) PURE;
};

FOLDER_INTERFACE(IEnumProperties, 0x01)
{
  // STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator) PURE;  
  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties) PURE;  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType) PURE;
};

FOLDER_INTERFACE(IFolderGetTypeID, 0x02)
{
  STDMETHOD(GetTypeID)(BSTR *name) PURE;
};

FOLDER_INTERFACE(IFolderGetPath, 0x03)
{
  STDMETHOD(GetPath)(BSTR *path) PURE;
};

FOLDER_INTERFACE(IFolderWasChanged, 0x04)
{
  STDMETHOD(WasChanged)(Int32 *wasChanged) PURE;
};

/*
FOLDER_INTERFACE(IFolderReload, 0x05)
{
  STDMETHOD(Reload)() PURE;
};
*/

FOLDER_INTERFACE_SUB(IFolderOperationsExtractCallback, IProgress, 0x06, 0x01)
{
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
};

/*
FOLDER_INTERFACE_SUB(IFolderOperationsUpdateCallback, IProgress, 0x06, 0x02)
{
  STDMETHOD(AskOverwrite)(
      const wchar_t *srcPath, 
      Int32 destIsFolder, 
      const FILETIME *destTime, 
      const UInt64 *destSize,
      const wchar_t *aDestPathRequest, 
      const wchar_t *aDestName, 
      BSTR *aDestPathResult, 
      Int32 *aResult);
};
*/

FOLDER_INTERFACE(IFolderOperations, 0x06)
{
  STDMETHOD(CreateFolder)(const wchar_t *name, IProgress *progress) PURE;
  STDMETHOD(CreateFile)(const wchar_t *name, IProgress *progress) PURE;
  STDMETHOD(Rename)(UInt32 index, const wchar_t *newName, IProgress *progress) PURE;
  STDMETHOD(Delete)(const UInt32 *indices, UInt32 numItems, IProgress *progress) PURE;
  STDMETHOD(CopyTo)(const UInt32 *indices, UInt32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback) PURE;
  STDMETHOD(MoveTo)(const UInt32 *indices, UInt32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback) PURE;
  STDMETHOD(CopyFrom)(const wchar_t *fromFolderPath,
      const wchar_t **itemsPaths, UInt32 numItems, IProgress *progress) PURE;
  STDMETHOD(SetProperty)(UInt32 index, PROPID propID, const PROPVARIANT *value, IProgress *progress) PURE;
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

/*
FOLDER_INTERFACE(IFolderOpen, 0x10)
{
  STDMETHOD(FolderOpen)(
    const wchar_t *aFileName, 
    // IArchiveHandler100 **anArchiveHandler, 
    // NZipRootRegistry::CArchiverInfo &anArchiverInfoResult,
    // UString &aDefaultName,
    IOpenArchive2CallBack *anOpenArchive2CallBack) PURE;
};
*/

#define FOLDER_MANAGER_INTERFACE(i, x) \
DEFINE_GUID(IID_ ## i, \
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x09, 0x00, x, 0x00, 0x00); \
struct i: public IUnknown
            
FOLDER_MANAGER_INTERFACE(IFolderManager, 0x00)
{
  STDMETHOD(OpenFolderFile)(const wchar_t *filePath, IFolderFolder **resultFolder, IProgress *progress) PURE;
  STDMETHOD(GetTypes)(BSTR *types) PURE;
  STDMETHOD(GetExtension)(const wchar_t *type, BSTR *extension) PURE;
  STDMETHOD(CreateFolderFile)(const wchar_t *type, const wchar_t *filePath, IProgress *progress) PURE;
};

FOLDER_MANAGER_INTERFACE(IFolderManagerGetIconPath, 0x01)
{
  STDMETHOD(GetIconPath)(const wchar_t *type, BSTR *iconPath) PURE;
};

/*
FOLDER_INTERFACE(IFolderExtract, 0x05, 0x0A);
{
  STDMETHOD(Clone)(IFolderFolder **aFolder) PURE;
};

FOLDER_INTERFACE(IFolderChangeNotify,0x05, 0x04, 0x00);
IFolderChangeNotify: public IUnknown
{
  STDMETHOD(OnChanged)() PURE;
};

FOLDER_INTERFACE(IFolderSetChangeNotify, 0x05, 0x05);
{
  STDMETHOD(SetChangeNotify)(IFolderChangeNotify *aChangeNotify) PURE;
};
*/


#endif
