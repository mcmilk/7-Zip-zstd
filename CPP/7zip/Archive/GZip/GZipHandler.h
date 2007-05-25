// GZip/Handler.h

#ifndef __GZIP_HANDLER_H
#define __GZIP_HANDLER_H

#include "Common/MyCom.h"

#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#include "GZipIn.h"
#include "GZipUpdate.h"

namespace NArchive {
namespace NGZip {

class CHandler: 
  public IInArchive,
  public IOutArchive,
  public ISetProperties,
  PUBLIC_ISetCompressCodecsInfo
  public CMyUnknownImp
{
public:
  MY_QUERYINTERFACE_BEGIN2(IInArchive)
  MY_QUERYINTERFACE_ENTRY(IOutArchive)
  MY_QUERYINTERFACE_ENTRY(ISetProperties)
  QUERY_ENTRY_ISetCompressCodecsInfo
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  STDMETHOD(Open)(IInStream *inStream, 
      const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *openArchiveCallback);  
  STDMETHOD(Close)();  
  
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems);  
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID,  PROPVARIANT *value);
  STDMETHOD(Extract)(const UInt32* indices, UInt32 numItems, 
      Int32 testMode, IArchiveExtractCallback *extractCallback);

  STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties);  
  STDMETHOD(GetArchivePropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);

  // IOutArchive

  STDMETHOD(UpdateItems)(ISequentialOutStream *outStream, UInt32 numItems,
      IArchiveUpdateCallback *updateCallback);

  STDMETHOD(GetFileTimeType)(UInt32 *timeType);  

  // ISetProperties
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProperties);

  DECL_ISetCompressCodecsInfo

  CHandler() { InitMethodProperties(); }

private:
  NArchive::NGZip::CItem m_Item;
  UInt64 m_StreamStartPosition;
  UInt64 m_DataOffset;
  UInt64 m_PackSize;
  CMyComPtr<IInStream> m_Stream;
  CCompressionMethodMode m_Method;
  UInt32 m_Level;

  DECL_EXTERNAL_CODECS_VARS

  void InitMethodProperties()
  {
    m_Method.NumMatchFinderCyclesDefined = false;
    m_Level = m_Method.NumPasses = m_Method.NumFastBytes = 
        m_Method.NumMatchFinderCycles = m_Method.Algo = 0xFFFFFFFF;
  }
};

}}

#endif
