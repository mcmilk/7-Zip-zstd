// 7z/Handler.h

#pragma once

#ifndef __7Z_HANDLER_H
#define __7Z_HANDLER_H

#include "../../Common/IArchiveHandler2.h"
#include "../../../Compress/Interface/CompressInterface.h"
#include "InEngine.h"
#include "ItemInfoUtils.h"

#include "Windows/PropVariant.h"

#include "CompressionMethod.h"

namespace NArchive {
namespace N7z {

#ifndef EXTRACT_ONLY

struct COneMethodInfo
{
  CObjectVector<CProperty> CoderProperties;
  CObjectVector<CProperty> EncoderProperties;
  bool MatchFinderIsDefined;
  CSysString MatchFinderName;
  AString MethodName;
};
#endif

// {23170F69-40C1-278A-1000-000110050000}
DEFINE_GUID(CLSID_CFormat7z, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x05, 0x00, 0x00);
class CHandler: 
  public IArchiveHandler200,
  #ifndef EXTRACT_ONLY
  public IOutArchiveHandler200,
  public ISetProperties,
  #endif
  public CComObjectRoot,
  public CComCoClass<CHandler, &CLSID_CFormat7z>
{
public:
BEGIN_COM_MAP(CHandler)
  COM_INTERFACE_ENTRY(IArchiveHandler200)
  #ifndef EXTRACT_ONLY
  COM_INTERFACE_ENTRY(IOutArchiveHandler200)
  COM_INTERFACE_ENTRY(ISetProperties)
  #endif
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHandler)

DECLARE_REGISTRY(CHandler, TEXT("SevenZip.Format7z.1"), 
                 TEXT("SevenZip.Format7z"), 0, THREADFLAGS_APARTMENT)

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

  #ifndef EXTRACT_ONLY
  // IOutArchiveHandler
  STDMETHOD(DeleteItems)(IOutStream *anOutStream, 
      const UINT32* anIndexes, UINT32 aNumItems, IUpdateCallBack *anUpdateCallBack);
  STDMETHOD(UpdateItems)(IOutStream *anOutStream, UINT32 aNumItems,
      IUpdateCallBack *anUpdateCallBack);

  STDMETHOD(GetFileTimeType)(UINT32 *aType);  

  // ISetProperties
  STDMETHOD(SetProperties)(const BSTR *aNames, const PROPVARIANT *aValues, INT32 aNumProperties);
  #endif

  CHandler();

private:
  CComPtr<IInStream> m_InStream;

  NArchive::N7z::CArchiveDatabaseEx m_Database;

  #ifndef EXTRACT_ONLY
  CObjectVector<COneMethodInfo> m_Methods;
  CRecordVector<CBind> m_Binds;
  bool m_Solid;
  bool m_CompressHeaders;
  UINT32 m_DefaultDicSize;
  UINT32 m_DefaultAlgorithm;
  UINT32 m_DefaultFastBytes;
  bool m_MultiThread;
  UINT32 m_MultiThreadMult;

  HRESULT SetCompressionMethod(CCompressionMethodMode &aMethod,
      CCompressionMethodMode &aHeaderMethod);
  #endif
  
  void Init()
  {
    #ifndef EXTRACT_ONLY
    m_Solid = true;
    m_CompressHeaders = true;
    m_MultiThread = false;
    m_DefaultDicSize = (1 << 20);
    m_DefaultAlgorithm = 1;
    m_DefaultFastBytes = 32;
    #endif
  }
};

}}

#endif