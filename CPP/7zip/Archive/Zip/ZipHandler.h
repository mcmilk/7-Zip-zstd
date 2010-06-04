// Zip/Handler.h

#ifndef __ZIP_HANDLER_H
#define __ZIP_HANDLER_H

#include "Common/DynamicBuffer.h"
#include "../../ICoder.h"
#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#include "ZipIn.h"
#include "ZipCompressionMode.h"

#ifndef _7ZIP_ST
#include "../../../Windows/System.h"
#endif

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

  int m_Level;
  int m_MainMethod;
  UInt32 m_DicSize;
  UInt32 m_Algo;
  UInt32 m_NumPasses;
  UInt32 m_NumFastBytes;
  UInt32 m_NumMatchFinderCycles;
  UInt32 m_MemSize;
  UInt32 m_Order;

  bool m_NumMatchFinderCyclesDefined;

  bool m_ForceAesMode;
  bool m_IsAesMode;
  Byte m_AesKeyMode;

  bool m_WriteNtfsTimeExtra;
  bool m_ForceLocal;
  bool m_ForceUtf8;

  #ifndef _7ZIP_ST
  UInt32 _numThreads;
  #endif

  DECL_EXTERNAL_CODECS_VARS

  void InitMethodProperties()
  {
    m_Level = -1;
    m_MainMethod = -1;
    m_Algo =
    m_DicSize =
    m_NumPasses =
    m_NumFastBytes =
    m_Order =
    m_MemSize =
    m_NumMatchFinderCycles = 0xFFFFFFFF;
    m_NumMatchFinderCyclesDefined = false;
    m_ForceAesMode = false;
    m_IsAesMode = false;
    m_AesKeyMode = 3; // aes-256
    m_WriteNtfsTimeExtra = true;
    m_ForceLocal = false;
    m_ForceUtf8 = false;
    #ifndef _7ZIP_ST
    _numThreads = NWindows::NSystem::GetNumberOfProcessors();;
    #endif
  }
};

}}

#endif
