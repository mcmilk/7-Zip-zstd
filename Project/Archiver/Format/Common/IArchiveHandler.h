// IArchiveHandler.h

#pragma once

#ifndef __IARCHIVEHANDLER_H
#define __IARCHIVEHANDLER_H

#include "Interface/IInOutStreams.h"
#include "Interface/IProgress.h"
#include "Interface/PropID.h"

namespace NFileTimeType
{
  enum EEnum
  {
    kWindows,
    kUnix,
    kDOS
  };
}


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
  STDMETHOD(Extract)(LPITEMIDLIST itemIDList, ISequentialOutStream **outStream, 
      INT32 askExtractMode) PURE;
  STDMETHOD(PrepareOperation)(INT32 askExtractMode) PURE;
  STDMETHOD(OperationResult)(INT32 resultEOperationResult) PURE;
};

// {23170F69-40C1-278A-0000-000100040000}
DEFINE_GUID(IID_IUpdateCallBack, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100040000")
IUpdateCallBack: public IProgress
{
public:
  STDMETHOD(GetUpdateItemInfo)(INT32 index, 
      INT32 *compress, // 1 - compress 0 - copy
      INT32 *existInArchive, // 1 - exist, 0 - not exist
      INT32 *indexInServer, // set if existInArchive == true
      UINT32 *attributes,
      FILETIME *creationTime, 
      FILETIME *lastAccessTime, 
      FILETIME *lastWriteTime, 
      UINT64 *size, 
      BSTR *name) PURE;

  STDMETHOD(CompressOperation)(INT32 index, IInStream **inStream) PURE;
  STDMETHOD(DeleteOperation)(LPITEMIDLIST itemIDList) PURE;

  STDMETHOD(OperationResult)(INT32 operationResult) PURE;
};

// {23170F69-40C1-278A-0000-000100040002}
DEFINE_GUID(IID_IUpdateCallBack2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x02);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100040002")
IUpdateCallBack2: public IUpdateCallBack
{
public:
  STDMETHOD(GetUpdateItemInfo2)(INT32 index, 
      INT32 *compress, // 1 - compress 0 - copy
      INT32 *existInArchive, // 1 - exist, 0 - not exist
      INT32 *indexInServer, // set if existInArchive == true
      UINT32 *attributes,
      FILETIME *creationTime, 
      FILETIME *lastAccessTime, 
      FILETIME *lastWriteTime, 
      UINT64 *size, 
      BSTR *name, 
      INT32 *isAnti) // 1 - File 0 - AntiFile) 
        PURE;
};

/*
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
      LPCITEMIDLIST itemIDList, 
      PROPID aPropID,  
      PROPVARIANT *values) PURE;


  STDMETHOD(DecompressItems)(const INT32* indices, INT32 numItems, INT32 aTestMode,
      IExtractCallBack *anExtractCallBack) PURE;
  STDMETHOD(DecompressAllItems)(INT32 aTestMode, 
      IExtractCallBack *anExtractCallBack) PURE;

};
*/

// {23170F69-40C1-278A-0000-000100010002}
DEFINE_GUID(IID_IOpenArchive2CallBack, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100010002")
IOpenArchive2CallBack: public IUnknown
{
public:
  STDMETHOD(SetTotal)(const UINT64 *files, const UINT64 *bytes) PURE;
  STDMETHOD(SetCompleted)(const UINT64 *files, const UINT64 *bytes) PURE;
};

/*
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
*/

inline INT32 BoolToMyBool(bool value)
  { return (value ? 1: 0); }

inline bool MyBoolToBool(INT32 value)
  { return (value != 0); }


// {23170F69-40C1-278A-0000-000100020001}
DEFINE_GUID(IID_IOutArchiveHandler, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100020001")
IOutArchiveHandler: public IUnknown
{
  STDMETHOD(DeleteItems)(IOutStream *outStream, const INT32* indices, INT32 numItems,
      IUpdateCallBack *updateCallback) PURE;
  STDMETHOD(UpdateItems)(IOutStream *outStream, INT32 numItems,
      INT32 storeMode, INT32 maximizeRatioMode,
      IUpdateCallBack *updateCallback) PURE;
  STDMETHOD(GetFileTimeType)(UINT32 *type) PURE;  
};

// {23170F69-40C1-278A-0000-000100030000}
DEFINE_GUID(IID_ISetProperty, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100030000")
ISetProperty: public IUnknown
{
  STDMETHOD(SetProperty)(BSTR name, const PROPVARIANT *value) PURE;
};

// {23170F69-40C1-278A-0000-000100030001}
DEFINE_GUID(IID_ISetProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100030001")
ISetProperties: public IUnknown
{
  STDMETHOD(SetProperties)(const BSTR *names, const PROPVARIANT *values, INT32 numProperties) PURE;
};

#endif
