// FolderInterface.h

#ifndef __FOLDERINTERFACE_H
#define __FOLDERINTERFACE_H

#include "../IProgress.h"

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

// {23170F69-40C1-278A-0000-000800000000}
DEFINE_GUID(IID_IFolderFolder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800000000")
IFolderFolder: public IUnknown
{
public:
  STDMETHOD(LoadItems)() = 0;
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems) = 0;  
  // STDMETHOD(GetNumberOfSubFolders)(UINT32 *numSubFolders) = 0;  
  STDMETHOD(GetProperty)(UINT32 itemIndex, PROPID propID, PROPVARIANT *value) = 0;
  STDMETHOD(BindToFolder)(UINT32 index, IFolderFolder **resultFolder) = 0;
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder) = 0;
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder) = 0;
  STDMETHOD(GetName)(BSTR *name) = 0;
};

// {23170F69-40C1-278A-0000-000800010000}
DEFINE_GUID(IID_IEnumProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800010000")
IEnumProperties: public IUnknown
{
public:
  // STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator) = 0;  
  STDMETHOD(GetNumberOfProperties)(UINT32 *numProperties) = 0;  
  STDMETHOD(GetPropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType) = 0;
};

// {23170F69-40C1-278A-0000-000800020000}
DEFINE_GUID(IID_IFolderGetTypeID, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800020000")
IFolderGetTypeID: public IUnknown
{
public:
  STDMETHOD(GetTypeID)(BSTR *name) = 0;
};

// {23170F69-40C1-278A-0000-000800030000}
DEFINE_GUID(IID_IFolderGetPath, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800030000")
IFolderGetPath: public IUnknown
{
public:
  STDMETHOD(GetPath)(BSTR *path) = 0;
};

// {23170F69-40C1-278A-0000-000800040000}
DEFINE_GUID(IID_IFolderWasChanged, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800040000")
IFolderWasChanged: public IUnknown
{
public:
  STDMETHOD(WasChanged)(INT32 *wasChanged) = 0;
};

/*
// {23170F69-40C1-278A-0000-000800050000}
DEFINE_GUID(IID_IFolderReload, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x05, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800050000")
IFolderReload: public IUnknown
{
public:
  STDMETHOD(Reload)() = 0;
};
*/

// {23170F69-40C1-278A-0000-000800060100}
DEFINE_GUID(IID_IFolderOperationsExtractCallback,
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x06, 0x01, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800060100")
IFolderOperationsExtractCallback: public IProgress
{
public:
  STDMETHOD(AskWrite)(
      const wchar_t *srcPath, 
      INT32 srcIsFolder, 
      const FILETIME *srcTime, 
      const UINT64 *srcSize,
      const wchar_t *destPathRequest, 
      BSTR *destPathResult, 
      INT32 *writeAnswer) = 0;
  STDMETHOD(ShowMessage)(const wchar_t *message) = 0;
};

/*
// {23170F69-40C1-278A-0000-000800060200}
DEFINE_GUID(IID_IFolderOperationsUpdateCallback,
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x06, 0x02, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800060200")
IFolderOperationsUpdateCallback: public IProgress
{
public:
  STDMETHOD(AskOverwrite)(
      const wchar_t *srcPath, 
      INT32 destIsFolder, 
      const FILETIME *destTime, 
      const UINT64 *destSize,
      const wchar_t *aDestPathRequest, 
      const wchar_t *aDestName, 
      BSTR *aDestPathResult, 
      INT32 *aResult);
};
*/

// {23170F69-40C1-278A-0000-000800060000}
DEFINE_GUID(IID_IFolderOperations, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x06, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800060000")
IFolderOperations: public IUnknown
{
public:
  STDMETHOD(CreateFolder)(const wchar_t *name, IProgress *progress) = 0;
  STDMETHOD(CreateFile)(const wchar_t *name, IProgress *progress) = 0;
  STDMETHOD(Rename)(UINT32 index, const wchar_t *newName, IProgress *progress) = 0;
  STDMETHOD(Delete)(const UINT32 *indices, UINT32 numItems, IProgress *progress) = 0;
  STDMETHOD(CopyTo)(const UINT32 *indices, UINT32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback) = 0;
  STDMETHOD(MoveTo)(const UINT32 *indices, UINT32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback) = 0;
  STDMETHOD(CopyFrom)(const wchar_t *fromFolderPath,
      const wchar_t **itemsPaths, UINT32 numItems, IProgress *progress) = 0;
  STDMETHOD(SetProperty)(UINT32 index, PROPID propID, const PROPVARIANT *value, IProgress *progress) = 0;
};

// {23170F69-40C1-278A-0000-000800070000}
DEFINE_GUID(IID_IFolderGetSystemIconIndex, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x07, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800070000")
IFolderGetSystemIconIndex: public IUnknown
{
public:
  STDMETHOD(GetSystemIconIndex)(UINT32 index, INT32 *iconIndex) = 0;
};

// {23170F69-40C1-278A-0000-000800080000}
DEFINE_GUID(IID_IFolderGetItemFullSize, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800080000")
IFolderGetItemFullSize: public IUnknown
{
public:
  STDMETHOD(GetItemFullSize)(UINT32 index, PROPVARIANT *value, IProgress *progress) = 0;
};

// {23170F69-40C1-278A-0000-000800090000}
DEFINE_GUID(IID_IFolderClone, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x09, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800090000")
IFolderClone: public IUnknown
{
public:
  STDMETHOD(Clone)(IFolderFolder **resultFolder) = 0;
};

/*
// {23170F69-40C1-278A-0000-0008000A0000}
DEFINE_GUID(IID_IFolderOpen, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x0A, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0008000A0000")
IFolderOpen: public IUnknown
{
  STDMETHOD(FolderOpen)(
    const wchar_t *aFileName, 
    // IArchiveHandler100 **anArchiveHandler, 
    // NZipRootRegistry::CArchiverInfo &anArchiverInfoResult,
    // UString &aDefaultName,
    IOpenArchive2CallBack *anOpenArchive2CallBack) = 0;
};
*/

// {23170F69-40C1-278A-0000-000900000000}
DEFINE_GUID(IID_IFolderManager, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000900000000")
IFolderManager: public IUnknown
{
  STDMETHOD(OpenFolderFile)(const wchar_t *filePath, IFolderFolder **resultFolder, IProgress *progress) = 0;
  STDMETHOD(GetTypes)(BSTR *types);
  STDMETHOD(GetExtension)(const wchar_t *type, BSTR *extension);
  STDMETHOD(CreateFolderFile)(const wchar_t *type, const wchar_t *filePath, IProgress *progress) = 0;
};

// {23170F69-40C1-278A-0000-000900010000}
DEFINE_GUID(IID_IFolderManagerGetIconPath, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x09, 0x00, 0x01, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000900010000")
IFolderManagerGetIconPath: public IUnknown
{
  STDMETHOD(GetIconPath)(const wchar_t *type, BSTR *iconPath) = 0;
};

/*
// {23170F69-40C1-278A-0000-000800050A00}
DEFINE_GUID(IID_IFolderExtract, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x05, 0x0A, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800050A00")
IFolderExtract: public IUnknown
{
public:
  STDMETHOD(Clone)(IFolderFolder **aFolder) = 0;
};
*/

/*
// {23170F69-40C1-278A-0000-000800050400}
DEFINE_GUID(IID_IFolderChangeNotify, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x05, 0x04, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800050400")
IFolderChangeNotify: public IUnknown
{
public:
  STDMETHOD(OnChanged)() = 0;
};

// {23170F69-40C1-278A-0000-000800050500}
DEFINE_GUID(IID_IFolderSetChangeNotify, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x05, 0x05, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000800050500")
IFolderSetChangeNotify: public IUnknown
{
public:
  STDMETHOD(SetChangeNotify)(IFolderChangeNotify *aChangeNotify) = 0;
};
*/


#endif
