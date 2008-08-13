// Lzma/Handler.h

#ifndef __GZIP_HANDLER_H
#define __GZIP_HANDLER_H

#include "Common/MyCom.h"

#include "../IArchive.h"
#include "../../Common/CreateCoder.h"

#include "LzmaIn.h"

namespace NArchive {
namespace NLzma {

// const UInt64 k_LZMA = 0x030101;

class CHandler:
  public IInArchive,
  PUBLIC_ISetCompressCodecsInfo
  public CMyUnknownImp
{
public:
  MY_QUERYINTERFACE_BEGIN
  MY_QUERYINTERFACE_ENTRY(IInArchive)
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

  UString GetMethodString();
public:
  CHandler() {  }

private:
  CHeader m_StreamInfo;
  UInt64 m_StreamStartPosition;
  UInt64 m_PackSize;

  CMyComPtr<IInStream> m_Stream;

  DECL_EXTERNAL_CODECS_VARS

  DECL_ISetCompressCodecsInfo

};

}}

#endif
