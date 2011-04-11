// Zip/Handler.h

#ifndef __ZIP_HANDLER_H
#define __ZIP_HANDLER_H

#include "Common/DynamicBuffer.h"
#include "../../ICoder.h"
#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#include "ZipIn.h"
#include "ZipCompressionMode.h"

namespace NArchive {
namespace NZip {

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

  CHandler();
private:
  CObjectVector<CItemEx> m_Items;
  CInArchive m_Archive;

  CBaseProps _props;

  int m_MainMethod;
  bool m_ForceAesMode;
  bool m_WriteNtfsTimeExtra;
  bool m_ForceLocal;
  bool m_ForceUtf8;

  DECL_EXTERNAL_CODECS_VARS

  void InitMethodProps()
  {
    _props.Init();
    m_MainMethod = -1;
    m_ForceAesMode = false;
    m_WriteNtfsTimeExtra = true;
    m_ForceLocal = false;
    m_ForceUtf8 = false;
  }
};

}}

#endif
