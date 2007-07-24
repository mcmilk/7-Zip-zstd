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

  INTERFACE_IInArchive(;)
  INTERFACE_IOutArchive(;)

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
