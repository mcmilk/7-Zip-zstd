// Zip/Handler.h

#pragma once

#ifndef __ZIP_HANDLER_H
#define __ZIP_HANDLER_H

#include "../Common/ArchiveInterface.h"
#include "Archive/Zip/InEngine.h"
#include "Common/DynamicBuffer.h"
#include "../../../Compress/Interface/CompressInterface.h"

#include "CompressionMethod.h"

// {23170F69-40C1-278A-1000-000110010000}
DEFINE_GUID(CLSID_CZipHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x01, 0x00, 0x00);

namespace NArchive {
namespace NZip {

class CZipHandler: 
  public IInArchive,
  public IOutArchive,
  public ISetProperties,
  public CComObjectRoot,
  public CComCoClass<CZipHandler,&CLSID_CZipHandler>
{
public:
BEGIN_COM_MAP(CZipHandler)
  COM_INTERFACE_ENTRY(IInArchive)
  COM_INTERFACE_ENTRY(IOutArchive)
  COM_INTERFACE_ENTRY(ISetProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CZipHandler)

DECLARE_REGISTRY(CZipHandler, 
    // TEXT("SevenZip.FormatZip.1"), TEXT("SevenZip.FormatZip"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"), 
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *aStream, 
      const UINT64 *aMaxCheckStartPosition,
      IArchiveOpenCallback *anOpenArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty);  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *anExtractCallback);
  STDMETHOD(ExtractAllItems)(INT32 testMode, 
      IArchiveExtractCallback *anExtractCallback);


  // IOutArchive
  STDMETHOD(UpdateItems)(IOutStream *outStream, UINT32 numItems,
      IArchiveUpdateCallback *updateCallback);
  STDMETHOD(GetFileTimeType)(UINT32 *timeType);  

  // ISetProperties
  STDMETHOD(SetProperties)(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties);

  CZipHandler();
private:
  CItemInfoExVector m_Items;
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