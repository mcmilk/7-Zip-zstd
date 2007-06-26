// 7z/Handler.h

#ifndef __7Z_HANDLER_H
#define __7Z_HANDLER_H

#include "../../ICoder.h"
#include "../IArchive.h"
#include "7zIn.h"

#include "7zCompressionMode.h"

#include "../../Common/CreateCoder.h"

#ifndef EXTRACT_ONLY
#include "../Common/HandlerOut.h"
#endif

namespace NArchive {
namespace N7z {

#ifdef _7Z_VOL
struct CRef
{
  int VolumeIndex;
  int ItemIndex;
};

/*
struct CRef2
{
  CRecordVector<CRef> Refs;
  UInt64 UnPackSize;
  UInt64 PackSize;
  UInt64 StartPos;
  CRef2(): UnPackSize(0), PackSize(0), StartPos(0) {}
};
*/

struct CVolume
{
  int StartRef2Index;
  CMyComPtr<IInStream> Stream;
  CArchiveDatabaseEx Database;
};
#endif

// {23170F69-40C1-278A-1000-000110070000}
DEFINE_GUID(CLSID_CFormat7z, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);

#ifndef __7Z_SET_PROPERTIES

#ifdef EXTRACT_ONLY
#ifdef COMPRESS_MT
#define __7Z_SET_PROPERTIES
#endif
#else 
#define __7Z_SET_PROPERTIES
#endif

#endif


class CHandler: 
  #ifndef EXTRACT_ONLY
  public NArchive::COutHandler,
  #endif
  public IInArchive,
  #ifdef _7Z_VOL
  public IInArchiveGetStream,
  #endif
  #ifdef __7Z_SET_PROPERTIES
  public ISetProperties, 
  #endif
  #ifndef EXTRACT_ONLY
  public IOutArchive, 
  #endif
  PUBLIC_ISetCompressCodecsInfo
  public CMyUnknownImp
{
public:
  MY_QUERYINTERFACE_BEGIN2(IInArchive)
  #ifdef _7Z_VOL
  MY_QUERYINTERFACE_ENTRY(IInArchiveGetStream)
  #endif
  #ifdef __7Z_SET_PROPERTIES
  MY_QUERYINTERFACE_ENTRY(ISetProperties)
  #endif
  #ifndef EXTRACT_ONLY
  MY_QUERYINTERFACE_ENTRY(IOutArchive)
  #endif
  QUERY_ENTRY_ISetCompressCodecsInfo
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE


  STDMETHOD(Open)(IInStream *stream, 
      const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems);  
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UInt32* indices, UInt32 numItems, 
      Int32 testMode, IArchiveExtractCallback *extractCallback);

  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  #ifdef _7Z_VOL
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);  
  #endif

  #ifdef __7Z_SET_PROPERTIES
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties);
  #endif

  #ifndef EXTRACT_ONLY
  // IOutArchiveHandler
  STDMETHOD(UpdateItems)(ISequentialOutStream *outStream, UInt32 numItems,
      IArchiveUpdateCallback *updateCallback);

  STDMETHOD(GetFileTimeType)(UInt32 *type);  

  // ISetProperties
  
  #endif

  DECL_ISetCompressCodecsInfo

  CHandler();

private:
  #ifdef _7Z_VOL
  CObjectVector<CVolume> _volumes;
  CObjectVector<CRef> _refs;
  #else
  CMyComPtr<IInStream> _inStream;
  NArchive::N7z::CArchiveDatabaseEx _database;
  #endif

  #ifdef EXTRACT_ONLY
  
  #ifdef COMPRESS_MT
  UInt32 _numThreads;
  #endif
  
  #else
  
  CRecordVector<CBind> _binds;

  HRESULT SetPassword(CCompressionMethodMode &methodMode, IArchiveUpdateCallback *updateCallback);

  HRESULT SetCompressionMethod(CCompressionMethodMode &method,
      CObjectVector<COneMethodInfo> &methodsInfo
      #ifdef COMPRESS_MT
      , UInt32 numThreads
      #endif
      );

  HRESULT SetCompressionMethod(
      CCompressionMethodMode &method,
      CCompressionMethodMode &headerMethod);

  #endif

  bool IsEncrypted(UInt32 index2) const;
  #ifndef _SFX

  CRecordVector<UInt64> _fileInfoPopIDs;
  void FillPopIDs();

  #endif
};

}}

#endif
