// DebHandler.h

#pragma once

#ifndef __DEB_HANDLER_H
#define __DEB_HANDLER_H

#include "../Common/ArchiveInterface.h"

#include "Archive/Deb/ItemInfoEx.h"

// {23170F69-40C1-278A-1000-0001100C0000}
DEFINE_GUID(CLSID_CFormatDeb, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x0C, 0x00, 0x00);

namespace NArchive {
namespace NDeb {

class CHandler: 
  public IInArchive,
  public CComObjectRoot,
  public CComCoClass<CHandler, &CLSID_CFormatDeb>
{
public:
BEGIN_COM_MAP(CHandler)
  COM_INTERFACE_ENTRY(IInArchive)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHandler)

DECLARE_REGISTRY(CHandler, 
    // TEXT("SevenZip.FormatDeb.1"), TEXT("SevenZip.FormatDeb"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"), 
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *stream, 
      const UINT64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *extractCallback);
  STDMETHOD(ExtractAllItems)(INT32 testMode, 
      IArchiveExtractCallback *extractCallback);

private:
  CObjectVector<NArchive::NDeb::CItemInfoEx> _items;
  CComPtr<IInStream> _inStream;
};

}}

#endif