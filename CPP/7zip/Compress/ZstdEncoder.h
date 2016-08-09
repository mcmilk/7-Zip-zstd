// ZstdEncoder.h
// (C) 2016 Rich Geldreich, Tino Reichardt

#define ZSTD_STATIC_LINKING_ONLY
#include "../../../C/Alloc.h"
#include "../../../C/ZStd/zstd.h"
#include "../../../C/ZStd/zbuff.h"

#include "../../Common/Common.h"
#include "../../Common/MyCom.h"
#include "../ICoder.h"
#include "../Common/StreamUtils.h"

#ifndef EXTRACT_ONLY
namespace NCompress {
namespace NZSTD {

struct CProps
{
  CProps() { clear (); }
  void clear ()
  {
    memset (this, 0, sizeof (*this));
    _ver_major = ZSTD_VERSION_MAJOR;
    _ver_minor = ZSTD_VERSION_MINOR;
    _level = 3;
  }

  Byte _ver_major;
  Byte _ver_minor;
  Byte _level;
  Byte _reserved[2];
};

class CEncoder:
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp
{
  ZBUFF_CCtx *_state;

  CProps _props;

  Byte *_inBuf;
  Byte *_outBuf;
  UInt32 _inPos;
  UInt32 _inSize;

  UInt32 _inBufSizeAllocated;
  UInt32 _outBufSizeAllocated;
  size_t _inBufSize;
  size_t _outBufSize;

  UInt64 _inSizeProcessed;
  UInt64 _outSizeProcessed;

  HRESULT CreateCompressor ();
  HRESULT CreateBuffers ();

public:
    MY_UNKNOWN_IMP2 (ICompressSetCoderProperties, ICompressWriteCoderProperties)
    STDMETHOD (Code) (ISequentialInStream * inStream, ISequentialOutStream *
      outStream, const UInt64 * inSize, const UInt64 * outSize,
      ICompressProgressInfo * progress);
    STDMETHOD (SetCoderProperties) (const PROPID * propIDs,
      const PROPVARIANT *props, UInt32 numProps);
    STDMETHOD (WriteCoderProperties) (ISequentialOutStream * outStream);

    CEncoder();
    virtual ~CEncoder();
};

}}
#endif
