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

#ifndef EXTRACT_ONLY
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

class CEncoder:
  public ICompressCoder,
  public ICompressSetCoderMt,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp
{
  CProps _props;

  UInt64 _processedIn;
  UInt64 _processedOut;
  UInt32 _inputSize;
  UInt32 _numThreads;

  Int32 _Long;
  Int32 _WindowLog;

  BROTLIMT_CCtx *_ctx;

public:
  MY_QUERYINTERFACE_BEGIN2(ICompressCoder)
  MY_QUERYINTERFACE_ENTRY(ICompressSetCoderMt)
  MY_QUERYINTERFACE_ENTRY(ICompressSetCoderProperties)
  MY_QUERYINTERFACE_ENTRY(ICompressWriteCoderProperties)
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  STDMETHOD (Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD (SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
  STDMETHOD (WriteCoderProperties)(ISequentialOutStream *outStream);
  STDMETHOD (SetNumberOfThreads)(UInt32 numThreads);

  CEncoder();
  virtual ~CEncoder();
};

}}
#endif
