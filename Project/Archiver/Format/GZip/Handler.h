// GZip/Handler.h

#pragma once

#ifndef __GZIP_HANDLER_H
#define __GZIP_HANDLER_H

#include "../../Common/IArchiveHandler2.h"

#include "Archive/GZip/InEngine.h"

#include "CompressionMethod.h"

// {23170F69-40C1-278A-1000-000110030000}
DEFINE_GUID(CLSID_CGZipHandler, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x03, 0x00, 0x00);

namespace NArchive {
namespace NGZip {

class CGZipHandler: 
  public IArchiveHandler200,
  public IOutArchiveHandler200,
  public ISetProperties,
  public CComObjectRoot,
  public CComCoClass<CGZipHandler,&CLSID_CGZipHandler>
{
public:
BEGIN_COM_MAP(CGZipHandler)
  COM_INTERFACE_ENTRY(IArchiveHandler200)
  COM_INTERFACE_ENTRY(IOutArchiveHandler200)
  COM_INTERFACE_ENTRY(ISetProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CGZipHandler)

DECLARE_REGISTRY(CGZipHandler, "SevenZip.FormatGZip.1", "SevenZip.FormatGZip", 0, THREADFLAGS_APARTMENT)
// DECLARE_REGISTRY(CGZipHandler, "", "", 0, THREADFLAGS_APARTMENT)


  STDMETHOD(Open)(IInStream *aStream, 
      const UINT64 *aMaxCheckStartPosition,
      IOpenArchive2CallBack *anOpenArchiveCallBack);  
  STDMETHOD(Close)();  
  
  STDMETHOD(GetNumberOfItems)(UINT32 *aNumItems);  
  STDMETHOD(GetProperty)(
      UINT32 anIndex, 
      PROPID aPropID,  
      PROPVARIANT *aValue);
  STDMETHOD(Extract)(const UINT32* anIndexes, UINT32 aNumItems, 
      INT32 aTestMode, IExtractCallback200 *anExtractCallBack);
  STDMETHOD(ExtractAllItems)(INT32 aTestMode, 
      IExtractCallback200 *anExtractCallBack);

  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty);

  // IOutArchiveHandler

  STDMETHOD(DeleteItems)(IOutStream *anOutStream, 
      const UINT32* anIndexes, UINT32 aNumItems, IUpdateCallBack *anUpdateCallBack);
  STDMETHOD(UpdateItems)(IOutStream *anOutStream, UINT32 aNumItems,
      IUpdateCallBack *anUpdateCallBack);

  STDMETHOD(GetFileTimeType)(UINT32 *aType);  

  // ISetProperties
  STDMETHOD(SetProperties)(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties);

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