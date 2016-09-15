// ZstdDecoder.h
// (C) 2016 Tino Reichardt

/**
 * you can define ZSTD_LEGACY_SUPPORT to be backwards compatible
 * with these versions: 0.5, 0.6, 0.7, 0.8 (0.8 == 1.0)
 *
 * /TR 2016-09-04
 */

#define ZSTD_STATIC_LINKING_ONLY
#include "../../../C/Alloc.h"
#include "../../../C/ZStd/zstd.h"

#include "../../Common/Common.h"
#include "../../Common/MyCom.h"
#include "../ICoder.h"
#include "../Common/StreamUtils.h"
#include "../Common/RegisterCodec.h"

namespace NCompress {
namespace NZSTD {

struct DProps
{
  DProps() { clear (); }
  void clear ()
  {
    memset(this, 0, sizeof (*this));
    _ver_major = ZSTD_VERSION_MAJOR;
    _ver_minor = ZSTD_VERSION_MINOR;
    _level = 1;
  }

  Byte _ver_major;
  Byte _ver_minor;
  Byte _level;
  Byte _reserved[2];
};

class CDecoder:public ICompressCoder,
  public ICompressSetDecoderProperties2, public ICompressSetBufSize,
#ifndef NO_READ_FROM_CODER
  public ICompressSetInStream,
  public ICompressSetOutStreamSize, public ISequentialInStream,
#endif
  public CMyUnknownImp
{
  CMyComPtr < ISequentialInStream > _inStream;

  DProps _props;

  ZSTD_DStream *_dstream;
  void *_buffIn;
  void *_buffOut;

  size_t _buffInSize;
  size_t _buffOutSize;
  size_t _buffInSizeAllocated;
  size_t _buffOutSizeAllocated;

  UInt64 _processedIn;
  UInt64 _processedOut;

  HRESULT CDecoder::CreateDecompressor();
  HRESULT CDecoder::ErrorOut(size_t code);
  HRESULT CodeSpec(ISequentialInStream *inStream, ISequentialOutStream *outStream, ICompressProgressInfo *progress);
  HRESULT SetOutStreamSizeResume(const UInt64 *outSize);

public:

  MY_QUERYINTERFACE_BEGIN2 (ICompressCoder)
  MY_QUERYINTERFACE_ENTRY (ICompressSetDecoderProperties2)
  MY_QUERYINTERFACE_ENTRY (ICompressSetBufSize)
#ifndef NO_READ_FROM_CODER
  MY_QUERYINTERFACE_ENTRY (ICompressSetInStream)
  MY_QUERYINTERFACE_ENTRY (ICompressSetOutStreamSize)
  MY_QUERYINTERFACE_ENTRY (ISequentialInStream)
#endif
  MY_QUERYINTERFACE_END

  MY_ADDREF_RELEASE
  STDMETHOD (Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD (SetDecoderProperties2) (const Byte *data, UInt32 size);
  STDMETHOD (SetOutStreamSize) (const UInt64 *outSize);
  STDMETHOD (SetInBufSize) (UInt32 streamIndex, UInt32 size);
  STDMETHOD (SetOutBufSize) (UInt32 streamIndex, UInt32 size);

#ifndef NO_READ_FROM_CODER
  STDMETHOD (SetInStream) (ISequentialInStream *inStream);
  STDMETHOD (ReleaseInStream) ();
  STDMETHOD (Read) (void *data, UInt32 size, UInt32 *processedSize);
  HRESULT CodeResume (ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress);
  UInt64 GetInputProcessedSize () const { return _processedIn; }
#endif

  CDecoder();
  virtual ~CDecoder();
};

}}
