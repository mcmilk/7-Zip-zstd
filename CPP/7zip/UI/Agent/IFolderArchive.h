// IFolderArchive.h

#ifndef __IFOLDER_ARCHIVE_H
#define __IFOLDER_ARCHIVE_H

#include "../../IDecl.h"
#include "../../Archive/IArchive.h"
#include "../../UI/Common/LoadCodecs.h"
#include "../../UI/FileManager/IFolder.h"

#include "../Common/ExtractMode.h"
#include "../Common/IFileExtractCallback.h"

#define FOLDER_ARCHIVE_INTERFACE_SUB(i, base, x) DECL_INTERFACE_SUB(i, base, 0x01, x)
#define FOLDER_ARCHIVE_INTERFACE(i, x) FOLDER_ARCHIVE_INTERFACE_SUB(i, IUnknown, x)

#define INTERFACE_IArchiveFolder(x) \
  STDMETHOD(Extract)(const UInt32 *indices, UInt32 numItems, \
      NExtract::NPathMode::EEnum pathMode, \
      NExtract::NOverwriteMode::EEnum overwriteMode, \
      const wchar_t *path, Int32 testMode, \
      IFolderArchiveExtractCallback *extractCallback2) x; \

FOLDER_ARCHIVE_INTERFACE(IArchiveFolder, 0x05)
{
  INTERFACE_IArchiveFolder(PURE)
};

#define INTERFACE_IInFolderArchive(x) \
  STDMETHOD(Open)(IInStream *inStream, const wchar_t *filePath, const wchar_t *arcFormat, BSTR *archiveTypeRes, IArchiveOpenCallback *openArchiveCallback) x; \
  STDMETHOD(ReOpen)(IArchiveOpenCallback *openArchiveCallback) x; \
  STDMETHOD(Close)() x; \
  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties) x; \
  STDMETHOD(GetPropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType) x; \
  STDMETHOD(BindToRootFolder)(IFolderFolder **resultFolder) x; \
  STDMETHOD(Extract)(NExtract::NPathMode::EEnum pathMode, \
      NExtract::NOverwriteMode::EEnum overwriteMode, const wchar_t *path, \
      Int32 testMode, IFolderArchiveExtractCallback *extractCallback2) x; \

FOLDER_ARCHIVE_INTERFACE(IInFolderArchive, 0x0E)
{
  INTERFACE_IInFolderArchive(PURE)
};

#define INTERFACE_IFolderArchiveUpdateCallback(x) \
  STDMETHOD(CompressOperation)(const wchar_t *name) x; \
  STDMETHOD(DeleteOperation)(const wchar_t *name) x; \
  STDMETHOD(OperationResult)(Int32 operationResult) x; \
  STDMETHOD(UpdateErrorMessage)(const wchar_t *message) x; \
  STDMETHOD(SetNumFiles)(UInt64 numFiles) x; \

FOLDER_ARCHIVE_INTERFACE_SUB(IFolderArchiveUpdateCallback, IProgress, 0x0B)
{
  INTERFACE_IFolderArchiveUpdateCallback(PURE)
};

#define INTERFACE_IOutFolderArchive(x) \
  STDMETHOD(SetFolder)(IFolderFolder *folder) x; \
  STDMETHOD(SetFiles)(const wchar_t *folderPrefix, const wchar_t **names, UInt32 numNames) x; \
  STDMETHOD(DeleteItems)(const wchar_t *newArchiveName, \
      const UInt32 *indices, UInt32 numItems, IFolderArchiveUpdateCallback *updateCallback) x; \
  STDMETHOD(DoOperation)(CCodecs *codecs, int index, \
      const wchar_t *newArchiveName, const Byte *stateActions, const wchar_t *sfxModule, \
      IFolderArchiveUpdateCallback *updateCallback) x; \
  STDMETHOD(DoOperation2)(const wchar_t *newArchiveName, const Byte *stateActions, \
      const wchar_t *sfxModule, IFolderArchiveUpdateCallback *updateCallback) x; \

FOLDER_ARCHIVE_INTERFACE(IOutFolderArchive, 0x0A)
{
  INTERFACE_IOutFolderArchive(PURE)
};

#endif
