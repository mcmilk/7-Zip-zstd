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

namespace NArchive
{
  namespace NExtract
  {
    namespace NAskMode
    {
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

// {23170F69-40C1-278A-0000-000100010000}
DEFINE_GUID(IID_IArchiveOpenCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100010000")
IArchiveOpenCallback: public IUnknown
{
public:
  STDMETHOD(SetTotal)(const UINT64 *files, const UINT64 *bytes) PURE;
  STDMETHOD(SetCompleted)(const UINT64 *files, const UINT64 *bytes) PURE;
};

// {23170F69-40C1-278A-0000-000100090000}
DEFINE_GUID(IID_IArchiveExtractCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100090000")
IArchiveExtractCallback: public IProgress
{
public:
  STDMETHOD(GetStream)(UINT32 index, ISequentialOutStream **outStream, 
      INT32 askExtractMode) PURE;
  STDMETHOD(PrepareOperation)(INT32 askExtractMode) PURE;
  STDMETHOD(SetOperationResult)(INT32 resultEOperationResult) PURE;
};


// {23170F69-40C1-278A-0000-0001000D0000}
DEFINE_GUID(IID_IArchiveOpenVolumeCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0D, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000D0000")
IArchiveOpenVolumeCallback: public IUnknown
{
public:
  STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream) PURE;
};

/*
// {23170F69-40C1-278A-0000-0001000E0000}
DEFINE_GUID(IID_IArchiveVolumeExtractCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000E0000")
IArchiveVolumeExtractCallback: public IUnknown
{
public:
  STDMETHOD(GetInStream)(const wchar_t *name, IInStream **inStream) PURE;
};
*/

/*
// {23170F69-40C1-278A-0000-0001000C0000}
DEFINE_GUID(IID_IArchivePropertiesInfo, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0C, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000C0000")
IArchivePropertiesInfo: public IUnknown
{
public:
};
*/

// {23170F69-40C1-278A-0000-000100080000}
DEFINE_GUID(IID_IInArchive, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100080000")
IInArchive: public IUnknown
{
public:
  STDMETHOD(Open)(IInStream *stream, const UINT64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback) PURE;  
  STDMETHOD(Close)() PURE;  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator) PURE;  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems) PURE;  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *extractCallback) PURE;
  STDMETHOD(ExtractAllItems)(INT32 testMode, IArchiveExtractCallback *extractCallback) PURE;
};


// {23170F69-40C1-278A-0000-000100040000}
DEFINE_GUID(IID_IArchiveUpdateCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100040000")
IArchiveUpdateCallback: public IProgress
{
public:
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator) PURE;  
  STDMETHOD(GetUpdateItemInfo)(UINT32 index, 
      INT32 *newData, // 1 - new data, 0 - old data
      INT32 *newProperties, // 1 - new properties, 0 - old properties
      UINT32 *indexInArchive // -1 if there is no in archive, or if doesn't matter
      ) PURE;
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(GetStream)(UINT32 index, IInStream **inStream) PURE;
  STDMETHOD(SetOperationResult)(INT32 operationResult) PURE;
};

// {23170F69-40C1-278A-0000-000100020000}
DEFINE_GUID(IID_IOutArchive, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100020000")
IOutArchive: public IUnknown
{
  STDMETHOD(UpdateItems)(IOutStream *outStream, UINT32 numItems,
      IArchiveUpdateCallback *updateCallback) PURE;
  STDMETHOD(GetFileTimeType)(UINT32 *type) PURE;  
};

// {23170F69-40C1-278A-0000-000100030000}
DEFINE_GUID(IID_ISetProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100030000")
ISetProperties: public IUnknown
{
  STDMETHOD(SetProperties)(const BSTR *names, const PROPVARIANT *values, INT32 numProperties) PURE;
};


#endif
