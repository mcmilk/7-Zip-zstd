// IArchiveHandler2.h

#pragma once

#ifndef __IARCHIVEHANDLER2_H
#define __IARCHIVEHANDLER2_H

#include "../Format/Common/IArchiveHandler.h"

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
      kSkipExisting
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
    kCancel
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
      const wchar_t *anExistName, const FILETIME *anExistTime, const UINT64 *anExistSize,
      const wchar_t *aNewName, const FILETIME *aNewTime, const UINT64 *aNewSize,
      INT32 *aResult);
  STDMETHOD(PrepareOperation)(const wchar_t *aName, INT32 anAskExtractMode) PURE;
  STDMETHOD(MessageError)(const wchar_t *aMessage) PURE;
  STDMETHOD(OperationResult)(INT32 anOperationResult) PURE;
};


// {23170F69-40C1-278A-0000-000100050000}
DEFINE_GUID(IID_IArchiveFolder, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100050000")
IArchiveFolder: public IUnknown
{
public:
  STDMETHOD(GetNumberOfItems)(UINT32 *aNumItems) PURE;  
  STDMETHOD(GetNumberOfSubFolders)(UINT32 *aNumSubFolders) PURE;  
  STDMETHOD(GetProperty)(UINT32 anItemIndex, PROPID aPropID, PROPVARIANT *aValue) PURE;
  STDMETHOD(BindToFolder)(UINT32 anIndex, IArchiveFolder **aFolder) PURE;
  STDMETHOD(BindToFolder)(const WCHAR *aFolderName, IArchiveFolder **aFolder) PURE;
  STDMETHOD(BindToParentFolder)(IArchiveFolder **aFolder) PURE;
  STDMETHOD(GetName)(BSTR *aName) PURE;

  STDMETHOD(Extract)(const UINT32 *anIndexes, UINT32 aNumItems, 
      NExtractionMode::NPath::EEnum aPathMode, 
      NExtractionMode::NOverwrite::EEnum anOverwriteMode, 
      const wchar_t *aPath,
      INT32 aTestMode,
      IExtractCallback2 *anExtractCallback2) PURE;
};

// {23170F69-40C1-278A-0000-000100060000}
DEFINE_GUID(IID_IArchiveHandler100, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100060000")
IArchiveHandler100: public IUnknown
{
public:
  STDMETHOD(Open)(IInStream *aStream, 
      const wchar_t *anItemDefaultName,
      const FILETIME *anItemDefaultTime,
      UINT32 anItemDefaultAttributes,
      const UINT64 *aMaxCheckStartPosition,
      const CLSID *aCLSID, 
      IOpenArchive2CallBack *anOpenArchiveCallBack) PURE;  
  STDMETHOD(ReOpen)(IInStream *aStream, 
      const wchar_t *anItemDefaultName,
      const FILETIME *anItemDefaultTime,
      UINT32 anItemDefaultAttributes,
      const UINT64 *aMaxCheckStartPosition,
      IOpenArchive2CallBack *anOpenArchiveCallBack) PURE;  
  STDMETHOD(Close)() PURE;  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty) PURE;  
  STDMETHOD(BindToRootFolder)(IArchiveFolder **aFolder) PURE;  
  STDMETHOD(Extract)(
      NExtractionMode::NPath::EEnum aPathMode, 
      NExtractionMode::NOverwrite::EEnum anOverwriteMode, 
      const wchar_t *aPath, 
      INT32 aTestMode,
      IExtractCallback2 *anExtractCallback2) PURE;
};

// {23170F69-40C1-278A-0000-0001000B0000}
DEFINE_GUID(IID_IUpdateCallback100, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000B0000")
IUpdateCallback100: public IProgress
{
public:
  STDMETHOD(CompressOperation)(const wchar_t *aName) PURE;
  STDMETHOD(DeleteOperation)(const wchar_t *aName) PURE;
  STDMETHOD(OperationResult)(INT32 aOperationResult) PURE;
};

// {23170F69-40C1-278A-0000-0001000A0000}
DEFINE_GUID(IID_IOutArchiveHandler100, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0A, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000A0000")
IOutArchiveHandler100: public IUnknown
{
  STDMETHOD(SetFolder)(IArchiveFolder *aFolder) PURE;
  STDMETHOD(SetFiles)(const wchar_t **aNames, UINT32 aNumNames) PURE;
  STDMETHOD(DeleteItems)(const wchar_t *aNewArchiveName, 
      const UINT32 *anIndexes, UINT32 aNumItems, IUpdateCallback100 *anUpdateCallback) PURE;
  STDMETHOD(DoOperation)(const CLSID *aCLSID, 
      const wchar_t *aNewArchiveName, const BYTE aStateActions[6],
      const wchar_t *aSfxModule,
      IUpdateCallback100 *anUpdateCallback) PURE;
};

// {23170F69-40C1-278A-0000-000100090000}
DEFINE_GUID(IID_IExtractCallback200, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100090000")
IExtractCallback200: public IProgress
{
public:
  STDMETHOD(Extract)(UINT32 anIndex, ISequentialOutStream **anOutStream, 
      INT32 anAskExtractMode) PURE;
  STDMETHOD(PrepareOperation)(INT32 anAskExtractMode) PURE;
  STDMETHOD(OperationResult)(INT32 aResultEOperationResult) PURE;
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
  STDMETHOD(GetPropertyInfo)(UINT32 anIndex, STATPROPSTG *aPropertyInfo) PURE;  
};
*/

// {23170F69-40C1-278A-0000-000100080000}
DEFINE_GUID(IID_IArchiveHandler200, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100080000")
IArchiveHandler200: public IUnknown
{
public:
  STDMETHOD(Open)(IInStream *aStream, const UINT64 *aMaxCheckStartPosition,
      IOpenArchive2CallBack *anOpenArchiveCallBack) PURE;  
  STDMETHOD(Close)() PURE;  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty);  
  STDMETHOD(GetNumberOfItems)(UINT32 *aNumItems) PURE;  
  STDMETHOD(GetProperty)(UINT32 anIndex, PROPID aPropID, PROPVARIANT *aValue) PURE;
  STDMETHOD(Extract)(const UINT32* anIndexes, UINT32 aNumItems, 
      INT32 aTestMode, IExtractCallback200 *anExtractCallBack) PURE;
  STDMETHOD(ExtractAllItems)(INT32 aTestMode, IExtractCallback200 *anExtractCallBack) PURE;
};

// {23170F69-40C1-278A-0000-000100020010}
DEFINE_GUID(IID_IOutArchiveHandler200, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x10);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100020010")
IOutArchiveHandler200: public IUnknown
{
  STDMETHOD(DeleteItems)(IOutStream *anOutStream, const UINT32* anIndexes, 
      UINT32 aNumItems, IUpdateCallBack *anUpdateCallBack) PURE;
  STDMETHOD(UpdateItems)(IOutStream *anOutStream, UINT32 aNumItems,
      IUpdateCallBack *anUpdateCallBack) PURE;
  STDMETHOD(GetFileTimeType)(UINT32 *aType) PURE;  
};


#endif
