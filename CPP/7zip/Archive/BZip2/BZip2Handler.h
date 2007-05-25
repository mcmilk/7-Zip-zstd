// BZip2/Handler.h

#ifndef __BZIP2_HANDLER_H
#define __BZIP2_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "../../Common/CreateCoder.h"
#include "BZip2Item.h"

#ifdef COMPRESS_MT
#include "../../../Windows/System.h"
#endif

namespace NArchive {
namespace NBZip2 {

class CHandler: 
  public IInArchive,
  public IOutArchive,
  public ISetProperties,
  PUBLIC_ISetCompressCodecsInfo
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  NArchive::NBZip2::CItem _item;
  UInt64 _streamStartPosition;

  UInt32 _level;
  UInt32 _dicSize;
  UInt32 _numPasses;
  #ifdef COMPRESS_MT
  UInt32 _numThreads;
  #endif

  DECL_EXTERNAL_CODECS_VARS

  void InitMethodProperties() 
  { 
    _level = 5;
    _dicSize = 
    _numPasses = 0xFFFFFFFF; 
    #ifdef COMPRESS_MT
    _numThreads = NWindows::NSystem::GetNumberOfProcessors();;
    #endif
  }

public:
  MY_QUERYINTERFACE_BEGIN2(IInArchive)
  MY_QUERYINTERFACE_ENTRY(IOutArchive)
  MY_QUERYINTERFACE_ENTRY(ISetProperties)
  QUERY_ENTRY_ISetCompressCodecsInfo
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  STDMETHOD(Open)(IInStream *stream, 
      const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems);  
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(Extract)(const UInt32* indices, UInt32 numItems, 
      Int32 testMode, IArchiveExtractCallback *extractCallback);
  
  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  // IOutArchiveHandler

  STDMETHOD(UpdateItems)(ISequentialOutStream *outStream, UInt32 numItems,
      IArchiveUpdateCallback *updateCallback);
  STDMETHOD(GetFileTimeType)(UInt32 *type);  

  // ISetProperties
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties);

  DECL_ISetCompressCodecsInfo

  CHandler() { InitMethodProperties(); }
};

}}

#endif
