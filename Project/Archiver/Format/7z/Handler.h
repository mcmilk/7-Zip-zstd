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

#ifndef _SFX
#include "RegistryInfo.h"
#endif

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
                 TEXT("SevenZip.Format7z"), UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Open)(IInStream *stream, 
      const UINT64 *maxCheckStartPosition,
      IOpenArchive2CallBack *openArchiveCallback);  
  STDMETHOD(Close)();  
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(Extract)(const UINT32* indices, UINT32 numItems, 
      INT32 testMode, IExtractCallback200 *extractCallback);
  STDMETHOD(ExtractAllItems)(INT32 testMode, IExtractCallback200 *extractCallback);

  #ifndef EXTRACT_ONLY
  // IOutArchiveHandler
  STDMETHOD(DeleteItems)(IOutStream *outStream, 
      const UINT32* indices, UINT32 numItems, IUpdateCallBack *updateCallback);
  STDMETHOD(UpdateItems)(IOutStream *outStream, UINT32 numItems,
      IUpdateCallBack *updateCallback);

  STDMETHOD(GetFileTimeType)(UINT32 *type);  

  // ISetProperties
  STDMETHOD(SetProperties)(const BSTR *names, const PROPVARIANT *values, INT32 numProperties);
  #endif

  CHandler();

private:
  CComPtr<IInStream> _inStream;

  NArchive::N7z::CArchiveDatabaseEx _database;

  #ifndef EXTRACT_ONLY
  CObjectVector<COneMethodInfo> _methods;
  CRecordVector<CBind> _binds;
  bool _solid;
  bool _compressHeaders;
  UINT32 _defaultDicSize;
  UINT32 _defaultAlgorithm;
  UINT32 _defaultFastBytes;
  bool _multiThread;
  UINT32 _multiThreadMult;
  AString _matchFinder;

  HRESULT SetParam(COneMethodInfo &oneMethodInfo, const UString &name, const UString &value);
  HRESULT SetParams(COneMethodInfo &oneMethodInfo, const UString &srcString);

  HRESULT SetCompressionMethod(CCompressionMethodMode &method,
      CCompressionMethodMode &headerMethod);
  #endif
  
  #ifndef _SFX

  NRegistryInfo::CMethodToCLSIDMap _methodMap;

  #endif


  void Init()
  {
    #ifndef EXTRACT_ONLY
    _solid = true;
    _compressHeaders = true;
    _multiThread = false;
    _defaultDicSize = (1 << 20);
    _defaultAlgorithm = 1;
    _defaultFastBytes = 32;
    _matchFinder = "BT4";
    #endif
  }
};

}}

#endif