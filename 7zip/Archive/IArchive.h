// IArchive.h

#ifndef __IARCHIVE_H
#define __IARCHIVE_H

#include "../IStream.h"
#include "../IProgress.h"
#include "../PropID.h"

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
  };

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
  STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes) PURE;
  STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes) PURE;
};

// {23170F69-40C1-278A-0000-000100090000}
DEFINE_GUID(IID_IArchiveExtractCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100090000")
IArchiveExtractCallback: public IProgress
{
public:
  STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, 
      Int32 askExtractMode) PURE;
  // GetStream OUT: S_OK - OK, S_FALSE - skeep this file
  STDMETHOD(PrepareOperation)(Int32 askExtractMode) PURE;
  STDMETHOD(SetOperationResult)(Int32 resultEOperationResult) PURE;
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


// {23170F69-40C1-278A-0000-0001000D0100}
DEFINE_GUID(IID_IArchiveOpenSetSubArchiveName, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0D, 0x01, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-0001000D0100")
IArchiveOpenSetSubArchiveName: public IUnknown
{
public:
  STDMETHOD(SetSubArchiveName)(const wchar_t *name) PURE;
};


// {23170F69-40C1-278A-0000-000100080000}
DEFINE_GUID(IID_IInArchive, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100080000")
IInArchive: public IUnknown
{
public:
  STDMETHOD(Open)(IInStream *stream, const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback) PURE;  
  STDMETHOD(Close)() PURE;  
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems) PURE;  
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(Extract)(const UInt32* indices, UInt32 numItems, 
      Int32 testMode, IArchiveExtractCallback *extractCallback) PURE;

  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value) PURE;

  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties) PURE;  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType) PURE;

  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties) PURE;  
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType) PURE;
};

// {23170F69-40C1-278A-0000-000100080100}
DEFINE_GUID(IID_IInArchiveGetStream, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x01, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100080100")
IInArchiveGetStream: public IUnknown
{
public:
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream) PURE;  
};


// {23170F69-40C1-278A-0000-000100040000}
DEFINE_GUID(IID_IArchiveUpdateCallback, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100040000")
IArchiveUpdateCallback: public IProgress
{
public:
  // STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator) PURE;  
  STDMETHOD(GetUpdateItemInfo)(UInt32 index, 
      Int32 *newData, // 1 - new data, 0 - old data
      Int32 *newProperties, // 1 - new properties, 0 - old properties
      UInt32 *indexInArchive // -1 if there is no in archive, or if doesn't matter
      ) PURE;
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value) PURE;
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **inStream) PURE;
  STDMETHOD(SetOperationResult)(Int32 operationResult) PURE;
};

// {23170F69-40C1-278A-0000-000100040002}
DEFINE_GUID(IID_IArchiveUpdateCallback2, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x02);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100040002")
IArchiveUpdateCallback2: public IArchiveUpdateCallback
{
public:
  STDMETHOD(GetVolumeSize)(UInt32 index, UInt64 *size) PURE;
  STDMETHOD(GetVolumeStream)(UInt32 index, ISequentialOutStream **volumeStream) PURE;
};

// {23170F69-40C1-278A-0000-000100020000}
DEFINE_GUID(IID_IOutArchive, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100020000")
IOutArchive: public IUnknown
{
  STDMETHOD(UpdateItems)(ISequentialOutStream *outStream, UInt32 numItems,
      IArchiveUpdateCallback *updateCallback) PURE;
  STDMETHOD(GetFileTimeType)(UInt32 *type) PURE;  
};

// {23170F69-40C1-278A-0000-000100030000}
DEFINE_GUID(IID_ISetProperties, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000100030000")
ISetProperties: public IUnknown
{
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties) PURE;
};


#endif
