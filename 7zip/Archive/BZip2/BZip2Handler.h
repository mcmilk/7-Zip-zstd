// BZip2/Handler.h

#pragma once

#ifndef __BZIP2_HANDLER_H
#define __BZIP2_HANDLER_H

#include "Common/MyCom.h"
#include "../IArchive.h"
#include "BZip2Item.h"

namespace NArchive {
namespace NBZip2 {

class CHandler: 
  public IInArchive,
  public IOutArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(
      IInArchive,
      IOutArchive
  )

  STDMETHOD(Open)(IInStream *stream, 
      const UINT64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *extractCallback);
  
  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UINT32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UINT32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);


  // IOutArchiveHandler

  STDMETHOD(UpdateItems)(IOutStream *outStream, UINT32 numItems,
      IArchiveUpdateCallback *updateCallback);

  STDMETHOD(GetFileTimeType)(UINT32 *type);  

private:
  CMyComPtr<IInStream> _stream;
  NArchive::NBZip2::CItem _item;
  UINT64 _streamStartPosition;
};

}}

#endif