// Rar/Handler.h

#pragma once

#ifndef __RAR_HANDLER_H
#define __RAR_HANDLER_H

#include "../Common/ArchiveInterface.h"
#include "Archive/Rar/InEngine.h"
#include "RarVolumeInStream.h"

// {23170F69-40C1-278B-0403-010000000000}
DEFINE_GUID(CLSID_CCompressRar15Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0403-020000000000}
DEFINE_GUID(CLSID_CCompressRar20Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0403-030000000000}
DEFINE_GUID(CLSID_CCompressRar29Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-06F1-0302000000000}
DEFINE_GUID(CLSID_CCryptoRar20Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-06F1-0303000000000}
DEFINE_GUID(CLSID_CCryptoRar29Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278A-1000-000110020000}
DEFINE_GUID(CLSID_CRarHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x02, 0x00, 0x00);

namespace NArchive {
namespace NRar {


/*
class CRarItemInfo: public NArchive::NRar::CItemInfoEx
{
public:
  // int VolumeIndex;
};
*/

class CHandler: 
  public IInArchive,
  public CComObjectRoot,
  public CComCoClass<CHandler,&CLSID_CRarHandler>
{
public:
BEGIN_COM_MAP(CHandler)
  COM_INTERFACE_ENTRY(IInArchive)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHandler)

DECLARE_REGISTRY(CHandler, 
    // "SevenZip.FormatRar.1", "SevenZip.FormatRar", 
    "SevenZip.1", "SevenZip", 
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *stream, 
      const UINT64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);	
  STDMETHOD(Close)();	
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);	
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *extractCallback);
  STDMETHOD(ExtractAllItems)(INT32 testMode, 
      IArchiveExtractCallback *extractCallback);

private:
  CRecordVector<CRefItem> _refItems;
  CObjectVector<NArchive::NRar::CItemInfoEx> _items;
  CObjectVector<NArchive::NRar::CInArchive> _archives;

  UINT64 GetPackSize(int refIndex) const;
  // NArchive::NRar::CInArchive _archive;
};

}}

#endif