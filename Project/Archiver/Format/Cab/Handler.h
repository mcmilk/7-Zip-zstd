// CabHandler.h

#pragma once

#ifndef __CAB_HANDLER_H
#define __CAB_HANDLER_H

#include "../Common/ArchiveInterface.h"
#include "Archive/Cab/InEngine.h"

// {23170F69-40C1-278A-1000-000110060000}
DEFINE_GUID(CLSID_CCabHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x06, 0x00, 0x00);

class CCabHandler: 
  public IInArchive,
  public CComObjectRoot,
  public CComCoClass<CCabHandler,&CLSID_CCabHandler>
{
public:
BEGIN_COM_MAP(CCabHandler)
  COM_INTERFACE_ENTRY(IInArchive)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCabHandler)

DECLARE_REGISTRY(CCabHandler, 
    // "SevenZip.FormatCab.1", "SevenZip.FormatCab", 
    "SevenZip.1", "SevenZip", 
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *inStream, 
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
  CObjectVector<NArchive::NCab::NHeader::CFolder> m_Folders;
  CObjectVector<NArchive::NCab::CFileInfo> m_Files;
  NArchive::NCab::CInArchiveInfo m_ArchiveInfo;
  CComPtr<IInStream> m_Stream;
};

#endif