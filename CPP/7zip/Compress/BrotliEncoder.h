// (C) 2017 Tino Reichardt

#define BROTLI_STATIC_LINKING_ONLY
#include "../../../C/Alloc.h"
#include "../../../C/Threads.h"
#include "../../../C/brotli/encode.h"
#include "../../../C/zstdmt/brotli-mt.h"

#include "../../Common/Common.h"
#include "../../Common/MyCom.h"
#include "../ICoder.h"
#include "../Common/StreamUtils.h"

#ifndef Z7_EXTRACT_ONLY
namespace NCompress {
namespace NBROTLI {

struct CProps
{
  CProps() { clear (); }
  void clear ()
  {
    memset(this, 0, sizeof (*this));
    _ver_major = BROTLI_VERSION_MAJOR;
    _ver_minor = BROTLI_VERSION_MINOR;
    _level = 3;
  }

  Byte _ver_major;
  Byte _ver_minor;
  Byte _level;
};

class CEncoder Z7_final:
  public ICompressCoder,
  public ICompressWriteCoderProperties,
  public ICompressSetCoderMt,
  public ICompressSetCoderProperties,
  public CMyUnknownImp
{
public:
  CProps _props;

  UInt64 _processedIn;
  UInt64 _processedOut;
  UInt32 _inputSize;
  UInt32 _numThreads;

  Int32 _Long;
  Int32 _WindowLog;

  BROTLIMT_CCtx *_ctx;

  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressWriteCoderProperties)
  Z7_COM_QI_ENTRY(ICompressSetCoderMt)
  Z7_COM_QI_ENTRY(ICompressSetCoderProperties)
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

public:

  UInt64 unpackSize;

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressWriteCoderProperties)
  Z7_IFACE_COM7_IMP(ICompressSetCoderMt)
  Z7_IFACE_COM7_IMP(ICompressSetCoderProperties)
  CEncoder();
  ~CEncoder();
};

}}
#endif
