// Tar/Handler.h

#pragma once

#ifndef __TAR_HANDLER_H
#define __TAR_HANDLER_H

#include "../Common/ArchiveInterface.h"

#include "Archive/Tar/ItemInfoEx.h"

// {23170F69-40C1-278A-1000-000110040000}
DEFINE_GUID(CLSID_CFormatTar, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x04, 0x00, 0x00);

namespace NArchive {
namespace NTar {

class CHandler: 
  public IInArchive,
  public IOutArchive,
  public CComObjectRoot,
  public CComCoClass<CHandler, &CLSID_CFormatTar>
{
public:
BEGIN_COM_MAP(CHandler)
  COM_INTERFACE_ENTRY(IInArchive)
  COM_INTERFACE_ENTRY(IOutArchive)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHandler)

DECLARE_REGISTRY(CHandler, 
    // TEXT("SevenZip.FormatTar.1"), TEXT("SevenZip.FormatTar"), 
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

  // IOutArchive
  STDMETHOD(UpdateItems)(IOutStream *outStream, UINT32 numItems,
      IArchiveUpdateCallback *updateCallback);
  STDMETHOD(GetFileTimeType)(UINT32 *type);  

private:
  CObjectVector<NArchive::NTar::CItemInfoEx> _items;
  CComPtr<IInStream> _inStream;
};

}}

#endif