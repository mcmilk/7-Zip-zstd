// cpio/Handler.h

#pragma once

#ifndef __cpio_HANDLER_H
#define __cpio_HANDLER_H

#include "../../Common/IArchiveHandler2.h"

#include "Archive/cpio/ItemInfoEx.h"

// {23170F69-40C1-278A-1000-000110080000}
DEFINE_GUID(CLSID_CFormatcpio, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x08, 0x00, 0x00);

namespace NArchive {
namespace Ncpio {

class CHandler: 
  public IArchiveHandler200,
  // public IOutArchiveHandler200,
  public CComObjectRoot,
  public CComCoClass<CHandler, &CLSID_CFormatcpio>
{
public:
BEGIN_COM_MAP(CHandler)
  COM_INTERFACE_ENTRY(IArchiveHandler200)
  // COM_INTERFACE_ENTRY(IOutArchiveHandler200)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHandler)

DECLARE_REGISTRY(CHandler, TEXT("SevenZip.Formatcpio.1"), 
    TEXT("SevenZip.Formatcpio"), 0, THREADFLAGS_APARTMENT)

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


  /*
  // IOutArchiveHandler
  STDMETHOD(DeleteItems)(IOutStream *anOutStream, 
      const UINT32* anIndexes, UINT32 aNumItems, IUpdateCallBack *anUpdateCallBack);
  STDMETHOD(UpdateItems)(IOutStream *anOutStream, UINT32 aNumItems,
      IUpdateCallBack *anUpdateCallBack);

  STDMETHOD(GetFileTimeType)(UINT32 *aType);  
  */

private:
  CObjectVector<CItemInfoEx> m_Items;
  CComPtr<IInStream> m_InStream;
};

}}

#endif