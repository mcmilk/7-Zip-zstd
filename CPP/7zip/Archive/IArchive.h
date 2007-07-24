// IArchive.h

#ifndef __IARCHIVE_H
#define __IARCHIVE_H

#include "../IStream.h"
#include "../IProgress.h"
#include "../PropID.h"

// MIDL_INTERFACE("23170F69-40C1-278A-0000-000600xx0000")
#define ARCHIVE_INTERFACE_SUB(i, base,  x) \
DEFINE_GUID(IID_ ## i, \
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x06, 0x00, x, 0x00, 0x00); \
struct i: public base

#define ARCHIVE_INTERFACE(i, x) ARCHIVE_INTERFACE_SUB(i, IUnknown, x)

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
  enum 
  {
    kName = 0,
    kClassID,
    kExtension,
    kAddExtension,
    kUpdate,
    kKeepName,
    kStartSignature,
    kFinishSignature,
    kAssociate
  };

  namespace NExtract
  {
    namespace NAskMode
    {
      enum 
      {
        kExtract = 0,
        kTest,
        kSkip
      };
    }
    namespace NOperationResult
    {
      enum 
      {
        kOK = 0,
        kUnSupportedMethod,
        kDataError,
        kCRCError
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
        kError
      };
    }
  }
}

ARCHIVE_INTERFACE(IArchiveOpenCallback, 0x10)
{
  STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes) PURE;
  STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes) PURE;
};


ARCHIVE_INTERFACE_SUB(IArchiveExtractCallback, IProgress, 0x20)
{
  STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, 
      Int32 askExtractMode) PURE;
  // GetStream OUT: S_OK - OK, S_FALSE - skeep this file
  STDMETHOD(PrepareOperation)(Int32 askExtractMode) PURE;
  STDMETHOD(SetOperationResult)(Int32 resultEOperationResult) PURE;
};


ARCHIVE_INTERFACE(IArchiveOpenVolumeCallback, 0x30)
{
  STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream) PURE;
};


ARCHIVE_INTERFACE(IInArchiveGetStream, 0x40)
{
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream) PURE;  
};


ARCHIVE_INTERFACE(IArchiveOpenSetSubArchiveName, 0x50)
{
  STDMETHOD(SetSubArchiveName)(const wchar_t *name) PURE;
};


/*
IInArchive::Extract:
  indices must be sorted 
  numItems = 0xFFFFFFFF means "all files"
  testMode != 0 means "test files without writing to outStream"
*/

#define INTERFACE_IInArchive(x) \
  STDMETHOD(Open)(IInStream *stream, const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *openArchiveCallback) x; \
  STDMETHOD(Close)() x; \
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems) x; \
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value) x; \
  STDMETHOD(Extract)(const UInt32* indices, UInt32 numItems, Int32 testMode, IArchiveExtractCallback *extractCallback) x; \
  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value) x; \
  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties) x; \
  STDMETHOD(GetPropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType) x; \
  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties) x; \
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType) x;

ARCHIVE_INTERFACE(IInArchive, 0x60)
{
  INTERFACE_IInArchive(PURE)
};


ARCHIVE_INTERFACE_SUB(IArchiveUpdateCallback, IProgress, 0x80)
{
  STDMETHOD(GetUpdateItemInfo)(UInt32 index, 
      Int32 *newData, // 1 - new data, 0 - old data
      Int32 *newProperties, // 1 - new properties, 0 - old properties
      UInt32 *indexInArchive // -1 if there is no in archive, or if doesn't matter
      ) PURE;
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **inStream) PURE;
  STDMETHOD(SetOperationResult)(Int32 operationResult) PURE;
};


ARCHIVE_INTERFACE_SUB(IArchiveUpdateCallback2, IArchiveUpdateCallback, 0x82)
{
  STDMETHOD(GetVolumeSize)(UInt32 index, UInt64 *size) PURE;
  STDMETHOD(GetVolumeStream)(UInt32 index, ISequentialOutStream **volumeStream) PURE;
};


#define INTERFACE_IOutArchive(x) \
  STDMETHOD(UpdateItems)(ISequentialOutStream *outStream, UInt32 numItems, IArchiveUpdateCallback *updateCallback) x; \
  STDMETHOD(GetFileTimeType)(UInt32 *type) x;

ARCHIVE_INTERFACE(IOutArchive, 0xA0)
{
  INTERFACE_IOutArchive(PURE)
};


ARCHIVE_INTERFACE(ISetProperties, 0x03)
{
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties) PURE;
};


#endif
