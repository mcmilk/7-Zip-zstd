// arj/Handler.h

#pragma once

#ifndef __ARJ_HANDLER_H
#define __ARJ_HANDLER_H

#include "../Common/ArchiveInterface.h"
#include "InEngine.h"

// {23170F69-40C1-278A-1000-0001100A0000}
DEFINE_GUID(CLSID_CArjHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x0A, 0x00, 0x00);

namespace NArchive {
namespace NArj {

class CHandler: 
  public IInArchive,
  public CComObjectRoot,
  public CComCoClass<CHandler, &CLSID_CArjHandler>
{
public:
BEGIN_COM_MAP(CHandler)
  COM_INTERFACE_ENTRY(IInArchive)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHandler)

DECLARE_REGISTRY(CHandler, 
    // TEXT("SevenZip.FormatArj.1"), TEXT("SevenZip.FormatArj"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"), 
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *inStream, 
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

  CHandler();
private:
  CObjectVector<CItemInfoEx> _items;
  CComPtr<IInStream> _stream;
};

}}

#endif