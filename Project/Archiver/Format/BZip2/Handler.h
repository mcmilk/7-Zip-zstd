// BZip2/Handler.h

#pragma once

#ifndef __BZIP2_HANDLER_H
#define __BZIP2_HANDLER_H

#include "../../Common/IArchiveHandler2.h"
#include "ItemInfoEx.h"

// {23170F69-40C1-278A-1000-000110070000}
DEFINE_GUID(CLSID_CBZip2Handler, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);

namespace NArchive {
namespace NBZip2 {

class CHandler: 
  public IArchiveHandler200,
  public IOutArchiveHandler200,
  public CComObjectRoot,
  public CComCoClass<CHandler,&CLSID_CBZip2Handler>
{
public:
BEGIN_COM_MAP(CHandler)
  COM_INTERFACE_ENTRY(IArchiveHandler200)
  COM_INTERFACE_ENTRY(IOutArchiveHandler200)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHandler)

DECLARE_REGISTRY(CHandler, "SevenZip.FormatBZip2.1", "SevenZip.FormatBZip2", 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *aStream, 
      const UINT64 *aMaxCheckStartPosition,
      IOpenArchive2CallBack *anOpenArchiveCallBack);  
  STDMETHOD(Close)();  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty);  
  STDMETHOD(GetNumberOfItems)(UINT32 *aNumItems);  
  STDMETHOD(GetProperty)(
      UINT32 anIndex, 
      PROPID aPropID,  
      PROPVARIANT *aValue);
  STDMETHOD(Extract)(const UINT32* anIndexes, UINT32 aNumItems, 
      INT32 aTestMode, IExtractCallback200 *anExtractCallBack);
  STDMETHOD(ExtractAllItems)(INT32 aTestMode, 
      IExtractCallback200 *anExtractCallBack);


  // IOutArchiveHandler

  STDMETHOD(DeleteItems)(IOutStream *anOutStream, 
      const UINT32* anIndexes, UINT32 aNumItems, IUpdateCallBack *anUpdateCallBack);
  STDMETHOD(UpdateItems)(IOutStream *anOutStream, UINT32 aNumItems,
      IUpdateCallBack *anUpdateCallBack);

  STDMETHOD(GetFileTimeType)(UINT32 *aType);  

private:
  CComPtr<IInStream> m_Stream;
  NArchive::NBZip2::CItemInfoEx m_Item;
  UINT64 m_StreamStartPosition;
};

}}

#endif