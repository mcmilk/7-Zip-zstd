// (C) 2016 Tino Reichardt

/**
 * you can define LZ5_LEGACY_SUPPORT to be backwards compatible (0.1 .. 0.7)
 * /TR 2016-10-01
 */

#define LZ5_STATIC_LINKING_ONLY
#include "../../../C/Alloc.h"
#include "../../../C/Threads.h"
#include "../../../C/lz5/lz5.h"
#include "../../../C/zstdmt/lz5mt.h"

#include "../../Windows/System.h"
#include "../../Common/Common.h"
#include "../../Common/MyCom.h"
#include "../ICoder.h"
#include "../Common/StreamUtils.h"
#include "../Common/RegisterCodec.h"
#include "../Common/ProgressMt.h"

struct Lz5Stream {
  ISequentialInStream *inStream;
  ISequentialOutStream *outStream;
  ICompressProgressInfo *progress;
  UInt64 *processedIn;
  UInt64 *processedOut;
  CCriticalSection *cs;
  int flags;
};

extern int Lz5Read(void *Stream, LZ5MT_Buffer * in);
extern int Lz5Write(void *Stream, LZ5MT_Buffer * in);

namespace NCompress {
namespace NLZ5 {

struct DProps
{
  DProps() { clear (); }
  void clear ()
  {
    memset(this, 0, sizeof (*this));
    _ver_major = LZ5_VERSION_MAJOR;
    _ver_minor = LZ5_VERSION_MINOR;
    _level = 1;
  }

  Byte _ver_major;
  Byte _ver_minor;
  Byte _level;
  Byte _reserved[2];
};

class CDecoder:public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp
{
  CMyComPtr < ISequentialInStream > _inStream;

  DProps _props;
  CCriticalSection cs;

  UInt64 _processedIn;
  UInt64 _processedOut;
  UInt32 _inputSize;
  UInt32 _numThreads;

  HRESULT CDecoder::ErrorOut(size_t code);
  HRESULT CodeSpec(ISequentialInStream *inStream, ISequentialOutStream *outStream, ICompressProgressInfo *progress);
  HRESULT SetOutStreamSizeResume(const UInt64 *outSize);

public:

  MY_QUERYINTERFACE_BEGIN2(ICompressCoder)
  MY_QUERYINTERFACE_ENTRY(ICompressSetDecoderProperties2)
#ifndef NO_READ_FROM_CODER
  MY_QUERYINTERFACE_ENTRY(ICompressSetInStream)
#endif
  MY_QUERYINTERFACE_END

  MY_ADDREF_RELEASE
  STDMETHOD (Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD (SetDecoderProperties2)(const Byte *data, UInt32 size);
  STDMETHOD (SetOutStreamSize)(const UInt64 *outSize);
  STDMETHODIMP CDecoder::SetNumberOfThreads(UInt32 numThreads);

#ifndef NO_READ_FROM_CODER
  STDMETHOD (SetInStream)(ISequentialInStream *inStream);
  STDMETHOD (ReleaseInStream)();
  UInt64 GetInputProcessedSize() const { return _processedIn; }
#endif
  HRESULT CodeResume(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress);

  CDecoder();
  virtual ~CDecoder();
};

}}
