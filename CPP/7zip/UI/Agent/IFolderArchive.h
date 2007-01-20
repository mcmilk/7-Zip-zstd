// IFolderArchive.h

#ifndef __IFOLDER_ARCHIVE_H
#define __IFOLDER_ARCHIVE_H

#include "../../Archive/IArchive.h"
// #include "../Format/Common/ArchiveInterface.h"
#include "../../FileManager/IFolder.h"
#include "../Common/IFileExtractCallback.h"
#include "../Common/ExtractMode.h"

// {23170F69-40C1-278A-0000-000100050000}
DEFINE_GUID(IID_IArchiveFolder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100050000")
IArchiveFolder: public IUnknown
{
public:
  STDMETHOD(Extract)(const UINT32 *indices, UINT32 numItems, 
      NExtract::NPathMode::EEnum pathMode, 
      NExtract::NOverwriteMode::EEnum overwriteMode, 
      const wchar_t *path,
      INT32 testMode,
      IFolderArchiveExtractCallback *extractCallback2) PURE;
};

// {23170F69-40C1-278A-0000-000100060000}
DEFINE_GUID(IID_IInFolderArchive, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100060000")
IInFolderArchive: public IUnknown
{
public:
  STDMETHOD(Open)(const wchar_t *filePath, 
      // CLSID *clsIDResult,
      BSTR *archiveType,
      IArchiveOpenCallback *openArchiveCallback) PURE;  
  STDMETHOD(ReOpen)(
      // const wchar_t *filePath, 
      IArchiveOpenCallback *openArchiveCallback) PURE;  
  STDMETHOD(Close)() PURE;  
  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(GetNumberOfProperties)(UINT32 *numProperties) PURE;  
  STDMETHOD(GetPropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType) PURE;
  STDMETHOD(GetNumberOfArchiveProperties)(UINT32 *numProperties) PURE;  
  STDMETHOD(GetArchivePropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType) PURE;
  STDMETHOD(BindToRootFolder)(IFolderFolder **resultFolder) PURE;  
  STDMETHOD(Extract)(
      NExtract::NPathMode::EEnum pathMode, 
      NExtract::NOverwriteMode::EEnum overwriteMode, 
      const wchar_t *path, 
      INT32 testMode,
      IFolderArchiveExtractCallback *extractCallback2) PURE;
};

// {23170F69-40C1-278A-0000-0001000B0000}
DEFINE_GUID(IID_IFolderArchiveUpdateCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000B0000")
IFolderArchiveUpdateCallback: public IProgress
{
public:
  STDMETHOD(CompressOperation)(const wchar_t *name) PURE;
  STDMETHOD(DeleteOperation)(const wchar_t *name) PURE;
  STDMETHOD(OperationResult)(INT32 operationResult) PURE;
  STDMETHOD(UpdateErrorMessage)(const wchar_t *message) PURE;
};

// {23170F69-40C1-278A-0000-0001000A0000}
DEFINE_GUID(IID_IOutFolderArchive, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0A, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000A0000")
IOutFolderArchive: public IUnknown
{
  STDMETHOD(SetFolder)(IFolderFolder *folder) PURE;
  STDMETHOD(SetFiles)(const wchar_t *folderPrefix, const wchar_t **names, UINT32 numNames) PURE;
  STDMETHOD(DeleteItems)(const wchar_t *newArchiveName, 
      const UINT32 *indices, UINT32 numItems, IFolderArchiveUpdateCallback *updateCallback) PURE;
  STDMETHOD(DoOperation)(
      const wchar_t *filePath, 
      const CLSID *clsID, 
      const wchar_t *newArchiveName, 
      const Byte *stateActions,
      const wchar_t *sfxModule,
      IFolderArchiveUpdateCallback *updateCallback) PURE;
};

#endif
