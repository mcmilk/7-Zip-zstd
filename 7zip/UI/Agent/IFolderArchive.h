// IFolderArchive.h

#pragma once

#ifndef __IFOLDERARCHIVE_H
#define __IFOLDERARCHIVE_H

#include "../../Archive/IArchive.h"
// #include "../Format/Common/ArchiveInterface.h"
#include "../../FileManager/IFolder.h"

namespace NExtractionMode {
  namespace NPath
  {
    enum EEnum
    {
      kFullPathnames,
      kCurrentPathnames,
      kNoPathnames
    };
  }
  namespace NOverwrite
  {
    enum EEnum
    {
      kAskBefore,
      kWithoutPrompt,
      kSkipExisting,
      kAutoRename
    };
  }
}

namespace NOverwriteAnswer
{
  enum EEnum
  {
    kYes,
    kYesToAll,
    kNo,
    kNoToAll,
    kAutoRename,
    kCancel,
  };
}


// {23170F69-40C1-278A-0000-000100070000}
DEFINE_GUID(IID_IFolderArchiveExtractCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100070000")
IFolderArchiveExtractCallback: public IProgress
{
public:
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UINT64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UINT64 *newSize,
      INT32 *answer);
  STDMETHOD(PrepareOperation)(const wchar_t *name, INT32 askExtractMode) PURE;
  STDMETHOD(MessageError)(const wchar_t *message) PURE;
  STDMETHOD(SetOperationResult)(INT32 operationResult) PURE;
};

// {23170F69-40C1-278A-0000-000100050000}
DEFINE_GUID(IID_IArchiveFolder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100050000")
IArchiveFolder: public IUnknown
{
public:
  STDMETHOD(Extract)(const UINT32 *indices, UINT32 numItems, 
      NExtractionMode::NPath::EEnum pathMode, 
      NExtractionMode::NOverwrite::EEnum overwriteMode, 
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
      NExtractionMode::NPath::EEnum pathMode, 
      NExtractionMode::NOverwrite::EEnum overwriteMode, 
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
      const BYTE *stateActions,
      const wchar_t *sfxModule,
      IFolderArchiveUpdateCallback *updateCallback) PURE;
};

#endif
