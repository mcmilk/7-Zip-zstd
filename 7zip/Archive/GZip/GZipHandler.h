// GZip/Handler.h

#pragma once

#ifndef __GZIP_HANDLER_H
#define __GZIP_HANDLER_H

#include "Common/MyCom.h"

#include "../IArchive.h"

#include "GZipIn.h"
#include "GZipUpdate.h"

namespace NArchive {
namespace NGZip {

class CHandler: 
  public IInArchive,
  public IOutArchive,
  public ISetProperties,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP3(
      IInArchive,
      IOutArchive,
      ISetProperties)

  STDMETHOD(Open)(IInStream *inStream, 
      const UINT64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *extractCallback);

  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UINT32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UINT32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  // IOutArchive

  STDMETHOD(UpdateItems)(IOutStream *outStream, UINT32 numItems,
      IArchiveUpdateCallback *updateCallback);

  STDMETHOD(GetFileTimeType)(UINT32 *timeType);  

  // ISetProperties
  STDMETHOD(SetProperties)(const BSTR *names, const PROPVARIANT *values, INT32 numProperties);

public:
  CHandler() { InitMethodProperties(); }

private:
  NArchive::NGZip::CItemEx m_Item;
  UINT64 m_StreamStartPosition;
  CMyComPtr<IInStream> m_Stream;
  CCompressionMethodMode m_Method;
  void InitMethodProperties()
  {
    m_Method.NumPasses = 1;
    m_Method.NumFastBytes = 32;
  }
};

}}

#endif
