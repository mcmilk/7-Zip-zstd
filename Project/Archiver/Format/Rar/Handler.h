// Rar/Handler.h

#pragma once

#ifndef __RAR_HANDLER_H
#define __RAR_HANDLER_H

#include "../../Common/IArchiveHandler2.h"
#include "Archive/Rar/InEngine.h"

// {23170F69-40C1-278B-0403-020000000000}
DEFINE_GUID(CLSID_CCompressRar20Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278A-1000-000250010000}
DEFINE_GUID(CLSID_CCryptoRar20Decoder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x50, 0x01, 0x00, 0x00);

// {23170F69-40C1-278A-1000-000110020000}
DEFINE_GUID(CLSID_CRarHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x02, 0x00, 0x00);
class CRarHandler: 
  public IArchiveHandler200,
  public CComObjectRoot,
  public CComCoClass<CRarHandler,&CLSID_CRarHandler>
{
public:
BEGIN_COM_MAP(CRarHandler)
  COM_INTERFACE_ENTRY(IArchiveHandler200)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CRarHandler)

DECLARE_REGISTRY(CRarHandler, "SevenZip.FormatRar.1", "SevenZip.FormatRar", 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *aStream, 
      const UINT64 *aMaxCheckStartPosition,
      IOpenArchive2CallBack *anOpenArchiveCallBack);	
  STDMETHOD(Close)();	
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty);	
  STDMETHOD(GetNumberOfItems)(UINT32 *aNumItems);  
  STDMETHOD(GetProperty)(UINT32 anIndex, PROPID aPropID,  PROPVARIANT *aValue);
  STDMETHOD(Extract)(const UINT32* anIndexes, UINT32 aNumItems, 
      INT32 aTestMode, IExtractCallback200 *anExtractCallBack);
  STDMETHOD(ExtractAllItems)(INT32 aTestMode, 
      IExtractCallback200 *anExtractCallBack);

private:
  CObjectVector<NArchive::NRar::CItemInfoEx> m_Items;
  NArchive::NRar::CInArchive m_Archive;
};

#endif