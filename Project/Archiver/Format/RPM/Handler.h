// RPM/Handler.h

#pragma once

#ifndef __RPM_HANDLER_H
#define __RPM_HANDLER_H

#include "../Common/ArchiveInterface.h"

// {23170F69-40C1-278A-1000-000110090000}
DEFINE_GUID(CLSID_CFormatRPM, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x09, 0x00, 0x00);

namespace NArchive {
namespace NRPM {

class CHandler: 
  public IInArchive,
  public CComObjectRoot,
  public CComCoClass<CHandler, &CLSID_CFormatRPM>
{
public:
BEGIN_COM_MAP(CHandler)
  COM_INTERFACE_ENTRY(IInArchive)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHandler)

DECLARE_REGISTRY(CHandler, 
    // TEXT("SevenZip.FormatRPM.1"), TEXT("SevenZip.FormatRPM"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"), 
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *inStream, 
      const UINT64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **anEnumProperty);  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IArchiveExtractCallback *extractCallback);
  STDMETHOD(ExtractAllItems)(INT32 testMode, 
      IArchiveExtractCallback *extractCallback);


private:
  CComPtr<IInStream> m_InStream;
  UINT64 m_Pos;
  UINT64 m_Size;
};

}}

#endif