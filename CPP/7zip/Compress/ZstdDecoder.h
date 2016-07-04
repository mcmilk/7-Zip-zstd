// ZstdDecoder.h
// (C) 2016 Rich Geldreich, Tino Reichardt

#include "StdAfx.h"

#define ZSTD_STATIC_LINKING_ONLY
#include "../../../C/Alloc.h"
#include "../../../C/ZStd/zstd.h"
#include "../../../C/ZStd/zbuff.h"
#include "../../../C/ZStd/zstd_legacy.h"

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
    memset (this, 0, sizeof (*this));
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
  Byte *_inBuf;
  Byte *_outBuf;
  UInt32 _inPos;
  UInt32 _inSize;
  bool _eofFlag;

  // ZBUFF_DCtx *_state;
  void *_state;

  DProps _props;
  bool _propsWereSet;

  bool _outSizeDefined;
  UInt64 _outSize;
  UInt64 _inSizeProcessed;
  UInt64 _outSizeProcessed;

  UInt32 _inBufSizeAllocated;
  UInt32 _outBufSizeAllocated;
  UInt32 _inBufSize;
  UInt32 _outBufSize;

  HRESULT CreateBuffers ();
  HRESULT CodeSpec (ISequentialInStream * inStream, ISequentialOutStream * outStream, ICompressProgressInfo * progress);
  HRESULT SetOutStreamSizeResume (const UInt64 * outSize);
  HRESULT CreateDecompressor ();

  // wrapper for different versions
  void *ZB_createDCtx(void);
  size_t ZB_freeDCtx(void *dctx);
  size_t ZB_decompressInit(void *dctx);
  size_t ZB_decompressContinue(void *dctx, void* dst, size_t *dstCapacityPtr, const void* src, size_t *srcSizePtr);

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
  STDMETHOD (Code)(ISequentialInStream * inStream, ISequentialOutStream * outStream, const UInt64 * inSize, const UInt64 * outSize, ICompressProgressInfo * progress);
  STDMETHOD (SetDecoderProperties2) (const Byte * data, UInt32 size);
  STDMETHOD (SetOutStreamSize) (const UInt64 * outSize);
  STDMETHOD (SetInBufSize) (UInt32 streamIndex, UInt32 size);
  STDMETHOD (SetOutBufSize) (UInt32 streamIndex, UInt32 size);

#ifndef NO_READ_FROM_CODER
  STDMETHOD (SetInStream) (ISequentialInStream * inStream);
  STDMETHOD (ReleaseInStream) ();
  STDMETHOD (Read) (void *data, UInt32 size, UInt32 * processedSize);
  HRESULT CodeResume (ISequentialOutStream * outStream, const UInt64 * outSize, ICompressProgressInfo * progress);

  UInt64 GetInputProcessedSize () const { return _inSizeProcessed; }
#endif

  CDecoder();
  virtual ~CDecoder();
};

}}
