// Archive/cpio/Handler.h

#pragma once

#ifndef __ARCHIVE_CPIO_HANDLER_H
#define __ARCHIVE_CPIO_HANDLER_H

#include "../Common/ArchiveInterface.h"

#include "Archive/cpio/ItemInfoEx.h"

// {23170F69-40C1-278A-1000-000110080000}
DEFINE_GUID(CLSID_CArchiveCpio, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x08, 0x00, 0x00);

namespace NArchive {
namespace Ncpio {

class CHandler: 
  public IInArchive,
  public CComObjectRoot,
  public CComCoClass<CHandler, &CLSID_CArchiveCpio>
{
public:
BEGIN_COM_MAP(CHandler)
  COM_INTERFACE_ENTRY(IInArchive)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHandler)

DECLARE_REGISTRY(CHandler, 
    // TEXT("SevenZip.Formatcpio.1"), TEXT("SevenZip.Formatcpio"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"), 
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *aStream, 
      const UINT64 *aMaxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetNumberOfItems)(UINT32 *aNumItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 aNumItems, 
      INT32 testMode, IArchiveExtractCallback *extractCallback);
  STDMETHOD(ExtractAllItems)(INT32 testMode, 
      IArchiveExtractCallback *extractCallback);

private:
  CObjectVector<CItemInfoEx> m_Items;
  CComPtr<IInStream> m_InStream;
};

}}

#endif