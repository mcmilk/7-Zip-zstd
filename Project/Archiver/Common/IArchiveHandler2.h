// IArchiveHandler2.h

#pragma once

#ifndef __IARCHIVEHANDLER2_H
#define __IARCHIVEHANDLER2_H

#include "../Format/Common/IArchiveHandler.h"

#include "../../FileManager/FolderInterface.h"

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
DEFINE_GUID(IID_IExtractCallback2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100070000")
IExtractCallback2: public IProgress
{
public:
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UINT64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UINT64 *newSize,
      INT32 *answer);
  STDMETHOD(PrepareOperation)(const wchar_t *name, INT32 askExtractMode) PURE;
  STDMETHOD(MessageError)(const wchar_t *message) PURE;
  STDMETHOD(OperationResult)(INT32 operationResult) PURE;
};


// {23170F69-40C1-278A-0000-000100070300}
DEFINE_GUID(IID_IExtractCallback3, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x07, 0x03, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100070300")
IExtractCallback3: public IExtractCallback2
{
public:
  STDMETHOD(AskWrite)(
      const wchar_t *srcPath, INT32 aSrcIsFolder, 
      const FILETIME *srcTime, const UINT64 *srcSize,
      const wchar_t *destPathRequest, BSTR *destPathResult, 
      INT32 *result);
};

// {23170F69-40C1-278A-0000-000100050000}
DEFINE_GUID(IID_IArchiveFolder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100050000")
IArchiveFolder: public IUnknown
{
public:
  /*
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems) PURE;  
  STDMETHOD(GetNumberOfSubFolders)(UINT32 *aNumSubFolders) PURE;  
  STDMETHOD(GetProperty)(UINT32 anItemIndex, PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(BindToFolder)(UINT32 index, IArchiveFolder **folder) PURE;
  STDMETHOD(BindToFolder)(const WCHAR *aFolderName, IArchiveFolder **folder) PURE;
  STDMETHOD(BindToParentFolder)(IArchiveFolder **folder) PURE;
  STDMETHOD(GetName)(BSTR *name) PURE;
  */

  STDMETHOD(Extract)(const UINT32 *indices, UINT32 numItems, 
      NExtractionMode::NPath::EEnum pathMode, 
      NExtractionMode::NOverwrite::EEnum overwriteMode, 
      const wchar_t *path,
      INT32 testMode,
      IExtractCallback2 *extractCallback2) PURE;
};

// {23170F69-40C1-278A-0000-000100060000}
DEFINE_GUID(IID_IArchiveHandler100, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100060000")
IArchiveHandler100: public IUnknown
{
public:
  STDMETHOD(Open)(IInStream *stream, 
      const wchar_t *itemDefaultName,
      const FILETIME *itemDefaultTime,
      UINT32 itemDefaultAttributes,
      const UINT64 *maxCheckStartPosition,
      const CLSID *clsID, 
      IOpenArchive2CallBack *openArchiveCallBack) PURE;  
  STDMETHOD(ReOpen)(IInStream *stream, 
      const wchar_t *itemDefaultName,
      const FILETIME *itemDefaultTime,
      UINT32 itemDefaultAttributes,
      const UINT64 *maxCheckStartPosition,
      IOpenArchive2CallBack *openArchiveCallBack) PURE;  
  STDMETHOD(Close)() PURE;  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator) PURE;  
  STDMETHOD(BindToRootFolder)(IFolderFolder **resultFolder) PURE;  
  STDMETHOD(Extract)(
      NExtractionMode::NPath::EEnum pathMode, 
      NExtractionMode::NOverwrite::EEnum overwriteMode, 
      const wchar_t *path, 
      INT32 testMode,
      IExtractCallback2 *extractCallback2) PURE;
};

// {23170F69-40C1-278A-0000-0001000B0000}
DEFINE_GUID(IID_IUpdateCallback100, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000B0000")
IUpdateCallback100: public IProgress
{
public:
  STDMETHOD(CompressOperation)(const wchar_t *name) PURE;
  STDMETHOD(DeleteOperation)(const wchar_t *name) PURE;
  STDMETHOD(OperationResult)(INT32 operationResult) PURE;
};

// {23170F69-40C1-278A-0000-0001000A0000}
DEFINE_GUID(IID_IOutArchiveHandler100, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0A, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000A0000")
IOutArchiveHandler100: public IUnknown
{
  STDMETHOD(SetFolder)(IFolderFolder *folder) PURE;
  STDMETHOD(SetFiles)(const wchar_t *folderPrefix, const wchar_t **names, UINT32 numNames) PURE;
  STDMETHOD(DeleteItems)(const wchar_t *newArchiveName, 
      const UINT32 *indices, UINT32 numItems, IUpdateCallback100 *updateCallback) PURE;
  STDMETHOD(DoOperation)(const CLSID *clsID, 
      const wchar_t *newArchiveName, const BYTE *stateActions,
      const wchar_t *sfxModule,
      IUpdateCallback100 *updateCallback) PURE;
};

// {23170F69-40C1-278A-0000-000100090000}
DEFINE_GUID(IID_IExtractCallback200, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100090000")
IExtractCallback200: public IProgress
{
public:
  STDMETHOD(Extract)(UINT32 index, ISequentialOutStream **outStream, 
      INT32 askExtractMode) PURE;
  STDMETHOD(PrepareOperation)(INT32 askExtractMode) PURE;
  STDMETHOD(OperationResult)(INT32 resultEOperationResult) PURE;
};

/*
// {23170F69-40C1-278A-0000-0001000C0000}
DEFINE_GUID(IID_IArchivePropertiesInfo, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0C, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000C0000")
IArchivePropertiesInfo: public IUnknown
{
public:
  STDMETHOD(GetNumberOfProperties)(UINT32 *aNumProperties) PURE;  
  STDMETHOD(GetPropertyInfo)(UINT32 index, STATPROPSTG *aPropertyInfo) PURE;  
};
*/

// {23170F69-40C1-278A-0000-000100080000}
DEFINE_GUID(IID_IArchiveHandler200, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100080000")
IArchiveHandler200: public IUnknown
{
public:
  STDMETHOD(Open)(IInStream *stream, const UINT64 *maxCheckStartPosition,
      IOpenArchive2CallBack *openArchiveCallBack) PURE;  
  STDMETHOD(Close)() PURE;  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems) PURE;  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IExtractCallback200 *extractCallback) PURE;
  STDMETHOD(ExtractAllItems)(INT32 testMode, IExtractCallback200 *extractCallback) PURE;
};

// {23170F69-40C1-278A-0000-000100020010}
DEFINE_GUID(IID_IOutArchiveHandler200, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x10);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100020010")
IOutArchiveHandler200: public IUnknown
{
  STDMETHOD(DeleteItems)(IOutStream *outStream, const UINT32* indices, 
      UINT32 numItems, IUpdateCallBack *updateCallback) PURE;
  STDMETHOD(UpdateItems)(IOutStream *outStream, UINT32 numItems,
      IUpdateCallBack *updateCallback) PURE;
  STDMETHOD(GetFileTimeType)(UINT32 *type) PURE;  
};


#endif
