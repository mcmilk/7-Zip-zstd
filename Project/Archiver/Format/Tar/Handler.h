// Tar/Handler.h

#pragma once

#ifndef __TAR_HANDLER_H
#define __TAR_HANDLER_H

#include "../../Common/IArchiveHandler2.h"

#include "Archive/Tar/ItemInfoEx.h"

// {23170F69-40C1-278A-1000-000110040000}
DEFINE_GUID(CLSID_CFormatTar, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x04, 0x00, 0x00);

namespace NArchive {
namespace NTar {

class CTarHandler: 
  public IArchiveHandler200,
  public IOutArchiveHandler200,
  public CComObjectRoot,
  public CComCoClass<CTarHandler, &CLSID_CFormatTar>
{
public:
BEGIN_COM_MAP(CTarHandler)
  COM_INTERFACE_ENTRY(IArchiveHandler200)
  COM_INTERFACE_ENTRY(IOutArchiveHandler200)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CTarHandler)

DECLARE_REGISTRY(CTarHandler, TEXT("SevenZip.FormatTar.1"), 
    TEXT("SevenZip.FormatTar"), 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *stream, 
      const UINT64 *maxCheckStartPosition,
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
  NArchive::NTar::CItemInfoExVector m_Items;
  CComPtr<IInStream> m_InStream;
};

}}

#endif