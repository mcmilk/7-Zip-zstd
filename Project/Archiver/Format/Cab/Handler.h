// CabHandler.h

#pragma once

#ifndef __CAB_HANDLER_H
#define __CAB_HANDLER_H

#include "../../Common/IArchiveHandler2.h"
#include "Archive/Cab/InEngine.h"


// {23170F69-40C1-278A-1000-000110060000}
DEFINE_GUID(CLSID_CCabHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x06, 0x00, 0x00);
class CCabHandler: 
  public IArchiveHandler200,
  public CComObjectRoot,
  public CComCoClass<CCabHandler,&CLSID_CCabHandler>
{
public:
BEGIN_COM_MAP(CCabHandler)
  COM_INTERFACE_ENTRY(IArchiveHandler200)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCabHandler)

DECLARE_REGISTRY(CCabHandler, "SevenZip.FormatCab.1", "SevenZip.FormatCab", 0, THREADFLAGS_APARTMENT)


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
  CObjectVector<NArchive::NCab::NHeader::CFolder> m_Folders;
  CObjectVector<NArchive::NCab::CFileInfo> m_Files;
  NArchive::NCab::CInArchiveInfo m_ArchiveInfo;
  CComPtr<IInStream> m_Stream;
};

#endif