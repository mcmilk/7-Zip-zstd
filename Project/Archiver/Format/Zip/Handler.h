// Zip/Handler.h

#pragma once

#ifndef __ZIP_HANDLER_H
#define __ZIP_HANDLER_H

#include "../../Common/IArchiveHandler2.h"
#include "Archive/Zip/InEngine.h"
#include "Common/DynamicBuffer.h"
#include "../../../Compress/Interface/CompressInterface.h"

#include "CompressionMethod.h"

// {23170F69-40C1-278A-1000-000110010000}
DEFINE_GUID(CLSID_CZipHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x01, 0x00, 0x00);

namespace NArchive {
namespace NZip {

class CZipHandler: 
  public IArchiveHandler200,
  public IOutArchiveHandler200,
  public ISetProperties,
  public CComObjectRoot,
  public CComCoClass<CZipHandler,&CLSID_CZipHandler>
{
public:
BEGIN_COM_MAP(CZipHandler)
  COM_INTERFACE_ENTRY(IArchiveHandler200)
  COM_INTERFACE_ENTRY(IOutArchiveHandler200)
  COM_INTERFACE_ENTRY(ISetProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CZipHandler)

DECLARE_REGISTRY(CZipHandler, "SevenZip.FormatZip.1", "SevenZip.FormatZip", 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *aStream, 
      const UINT64 *aMaxCheckStartPosition,
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

  // ISetProperties
  STDMETHOD(SetProperties)(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties);

  CZipHandler();
private:
  CItemInfoExVector m_Items;
  CInArchive m_Archive;
  bool m_ArchiveIsOpen;
  CCompressionMethodMode m_Method;
  void InitMethodProperties()
  {
    m_Method.NumPasses = 1;
    m_Method.NumFastBytes = 32;
  }
};

}}

#endif