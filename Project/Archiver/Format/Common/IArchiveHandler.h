// IArchiveHandler.h

#pragma once

#ifndef __IARCHIVEHANDLER_H
#define __IARCHIVEHANDLER_H

#include "Interface/IInOutStreams.h"
#include "Interface/IProgress.h"

namespace NFileTimeType
{
  enum EEnum
  {
    kWindows,
    kUnix,
    kDOS
  };
}

enum // CArchiveItemPropertyID
{
  kaipidHandlerItemIndex = 2,
  kaipidPath,
  kaipidName,
  kaipidExtension,
  kaipidIsFolder,
  kaipidSize,
  kaipidPackedSize,
  kaipidAttributes,
  kaipidCreationTime,
  kaipidLastAccessTime,
  kaipidLastWriteTime,
  kaipidSolid, 
  kaipidComment, 
  kaipidEncrypted, 
  kaipidSplitBefore, 
  kaipidSplitAfter, 
  kaipidDictionarySize, 
  kaipidCRC, 
  kaipidType, 
  kaipidUserDefined = 0x10000
};

namespace NArchiveHandler{
  namespace NExtract{
    
    namespace NAskMode{
      enum 
      {
        kExtract = 0,
        kTest,
        kSkip,
      };
    }
    
    namespace NOperationResult
    {
      enum 
      {
        kOK = 0,
        kUnSupportedMethod,
        kDataError,
        kCRCError,
      };
    }
  }
  namespace NUpdate
  {
    /*
    namespace NOperationType
    {
      enum 
      {
        kAdding = 0,
        kUpdating,
        kDeleting,
      };
    }
    */
    namespace NOperationResult
    {
      enum 
      {
        kOK = 0,
        kError,
      };
    }
  }

}

// {23170F69-40C1-278A-0000-000100030000}
DEFINE_GUID(IID_IExtractCallBack, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100030000")
IExtractCallBack: public IProgress
{
public:
  STDMETHOD(Extract)(LPITEMIDLIST anItemIDList, ISequentialOutStream **anOutStream, 
      INT32 anAskExtractMode) PURE;
  STDMETHOD(PrepareOperation)(INT32 anAskExtractMode) PURE;
  STDMETHOD(OperationResult)(INT32 aResultEOperationResult) PURE;
};

// {23170F69-40C1-278A-0000-000100040000}
DEFINE_GUID(IID_IUpdateCallBack, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100040000")
IUpdateCallBack: public IProgress
{
public:
  STDMETHOD(GetUpdateItemInfo)(INT32 anIndex, 
      INT32 *anCompress, // 1 - compress 0 - copy
      INT32 *anExistInArchive, // 1 - exist, 0 - not exist
      INT32 *anIndexInServer, // set if anExistInArchive == true
      UINT32 *anAttributes,
      FILETIME *aCreationTime, 
      FILETIME *aLastAccessTime, 
      FILETIME *aLastWriteTime, 
      UINT64 *aSize, 
      BSTR *aName) PURE;

  STDMETHOD(CompressOperation)(INT32 anIndex, IInStream **anInStream) PURE;
  STDMETHOD(DeleteOperation)(LPITEMIDLIST anItemIDList) PURE;

  STDMETHOD(OperationResult)(INT32 aOperationResult) PURE;
};

// {23170F69-40C1-278A-0000-000100010001}
DEFINE_GUID(IID_IArchiveHandler, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100010001")
IArchiveHandler: public IUnknown
{
public:
  STDMETHOD(IsArchive)(IInStream *aStream) PURE;  
  STDMETHOD(CloseArchive)() PURE;  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty) PURE;  
  STDMETHOD(EnumObjects)(LPENUMIDLIST *anEnumIDList) PURE;  

  STDMETHOD(GetPropertyValue)(
      LPCITEMIDLIST anItemIDList, 
      PROPID aPropID,  
      PROPVARIANT *aValues) PURE;


  STDMETHOD(DecompressItems)(const INT32* anIndexes, INT32 aNumItems, INT32 aTestMode,
      IExtractCallBack *anExtractCallBack) PURE;
  STDMETHOD(DecompressAllItems)(INT32 aTestMode, 
      IExtractCallBack *anExtractCallBack) PURE;

};

// {23170F69-40C1-278A-0000-000100010002}
DEFINE_GUID(IID_IOpenArchive2CallBack, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100010002")
IOpenArchive2CallBack: public IUnknown
{
public:
  STDMETHOD(SetTotal)(const UINT64 *aFiles, const UINT64 *aBytes) PURE;
  STDMETHOD(SetCompleted)(const UINT64 *aFiles, const UINT64 *aBytes) PURE;
};

// {23170F69-40C1-278A-0000-000100010003}
DEFINE_GUID(IID_IArchiveHandler2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100010003")
IArchiveHandler2: public IArchiveHandler
{
public:
  STDMETHOD(IsArchive2)(IInStream *aStream,
      IOpenArchive2CallBack *anOpenArchiveCallBack) PURE;  
};

// {23170F69-40C1-278A-0000-000100010004}
DEFINE_GUID(IID_IArchiveHandler4, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x04);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100010004")
IArchiveHandler4: public IArchiveHandler2
{
public:
  STDMETHOD(IsArchive4)(IInStream *aStream, const UINT64 *aMaxCheckStartPosition,
      IOpenArchive2CallBack *anOpenArchiveCallBack) PURE;  
};

inline INT32 BoolToMyBool(bool aValue)
  { return (aValue ? 1: 0); }

inline bool MyBoolToBool(INT32 aValue)
  { return (aValue != 0); }


// {23170F69-40C1-278A-0000-000100020001}
DEFINE_GUID(IID_IOutArchiveHandler, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100020001")
IOutArchiveHandler: public IUnknown
{
  STDMETHOD(DeleteItems)(IOutStream *anOutStream, const INT32* anIndexes, INT32 aNumItems,
      IUpdateCallBack *anUpdateCallBack) PURE;
  STDMETHOD(UpdateItems)(IOutStream *anOutStream, INT32 aNumItems,
      INT32 aStoreMode, INT32 aMaximizeRatioMode,
      IUpdateCallBack *anUpdateCallBack) PURE;
  STDMETHOD(GetFileTimeType)(UINT32 *aType) PURE;  
};

// {23170F69-40C1-278A-0000-000100030000}
DEFINE_GUID(IID_ISetProperty, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100030000")
ISetProperty: public IUnknown
{
  STDMETHOD(SetProperty)(BSTR aName, const PROPVARIANT *aValue) PURE;
};

// {23170F69-40C1-278A-0000-000100030001}
DEFINE_GUID(IID_ISetProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100030001")
ISetProperties: public IUnknown
{
  STDMETHOD(SetProperties)(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties) PURE;
};

#endif
