// Split/Handler.h

#ifndef __SPLIT_HANDLER_H
#define __SPLIT_HANDLER_H

#include "Common/MyCom.h"
#include "Common/String.h"
#include "../IArchive.h"

namespace NArchive {
namespace NSplit {

class CHandler: 
  public IInArchive,
  public IInArchiveGetStream,
  // public IOutArchive, 
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchiveGetStream)

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

  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);  

  // IOutArchiveHandler
  STDMETHOD(UpdateItems)(ISequentialOutStream *outStream, UInt32 numItems,
      IArchiveUpdateCallback *updateCallback);

  STDMETHOD(GetFileTimeType)(UInt32 *type);  

private:
  UString _subName;
  UString _name;
  CObjectVector<CMyComPtr<IInStream> > _streams;
  CRecordVector<UInt64> _sizes;

  UInt64 _totalSize;
};

}}

#endif