// Zip/Handler.h

#pragma once

#ifndef __ZIP_HANDLER_H
#define __ZIP_HANDLER_H

#include "Common/DynamicBuffer.h"
#include "../../ICoder.h"
#include "../IArchive.h"

#include "ZipIn.h"
#include "ZipCompressionMode.h"

namespace NArchive {
namespace NZip {

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
      ISetProperties
      )

  STDMETHOD(Open)(IInStream *aStream, 
      const UINT64 *aMaxCheckStartPosition,
      IArchiveOpenCallback *anOpenArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *anExtractCallback);

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

  CHandler();
private:
  CObjectVector<CItemEx> m_Items;
  CInArchive m_Archive;
  bool m_ArchiveIsOpen;
  CCompressionMethodMode m_Method;
  void InitMethodProperties()
  {
    m_Method.NumPasses = 1;
    m_Method.NumFastBytes = 32;
  }
};

}}

#endif
