// GZip/Handler.h

#pragma once

#ifndef __GZIP_HANDLER_H
#define __GZIP_HANDLER_H

#include "../Common/ArchiveInterface.h"

#include "Archive/GZip/InEngine.h"

#include "CompressionMethod.h"

// {23170F69-40C1-278A-1000-000110030000}
DEFINE_GUID(CLSID_CGZipHandler, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x03, 0x00, 0x00);

namespace NArchive {
namespace NGZip {

class CGZipHandler: 
  public IInArchive,
  public IOutArchive,
  public ISetProperties,
  public CComObjectRoot,
  public CComCoClass<CGZipHandler,&CLSID_CGZipHandler>
{
public:
BEGIN_COM_MAP(CGZipHandler)
  COM_INTERFACE_ENTRY(IInArchive)
  COM_INTERFACE_ENTRY(IOutArchive)
  COM_INTERFACE_ENTRY(ISetProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CGZipHandler)

DECLARE_REGISTRY(CGZipHandler, 
    // TEXT("SevenZip.FormatGZip.1"), TEXT("SevenZip.FormatGZip"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"), 
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *inStream, 
      const UINT64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *extractCallback);
  STDMETHOD(ExtractAllItems)(INT32 testMode, 
      IArchiveExtractCallback *extractCallback);

  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty);

  // IOutArchive

  STDMETHOD(UpdateItems)(IOutStream *outStream, UINT32 numItems,
      IArchiveUpdateCallback *updateCallback);

  STDMETHOD(GetFileTimeType)(UINT32 *timeType);  

  // ISetProperties
  STDMETHOD(SetProperties)(const BSTR *names, const PROPVARIANT *values, INT32 numProperties);

public:
  CGZipHandler()
    { InitMethodProperties(); }
private:

  NArchive::NGZip::CItemInfoEx m_Item;
  UINT64 m_StreamStartPosition;
  CComPtr<IInStream> m_Stream;

  CCompressionMethodMode m_Method;
  void InitMethodProperties()
  {
    m_Method.NumPasses = 1;
    m_Method.NumFastBytes = 32;
  }
};

}}

#endif