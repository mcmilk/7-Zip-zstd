// (C) 2016 - 2018 Tino Reichardt

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#endif

#include "StdAfx.h"
#include "ZstdDecoder.h"

namespace NCompress {
namespace NZSTD {

CDecoder::CDecoder():
  _ctx(NULL),
  _srcBuf(NULL),
  _dstBuf(NULL),
  _srcBufSize(ZSTD_DStreamInSize()),
  _dstBufSize(ZSTD_DStreamOutSize()),
  _processedIn(0),
  _processedOut(0)
{
  _props.clear();
}

CDecoder::~CDecoder()
{
  if (_ctx) {
    ZSTD_freeDCtx(_ctx);
    MyFree(_srcBuf);
    MyFree(_dstBuf);
  }
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * prop, UInt32 size)
{
  DProps *pProps = (DProps *)prop;

  switch (size) {
  case 3:
  case 5:
    memcpy(&_props, pProps, sizeof (DProps));
    return S_OK;
  default:
    return E_NOTIMPL;
  }
}

HRESULT CDecoder::SetOutStreamSizeResume(const UInt64 * /*outSize*/)
{
  _processedOut = 0;
  return S_OK;
}

STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 * outSize)
{
  _processedIn = 0;
  RINOK(SetOutStreamSizeResume(outSize));
  return S_OK;
}

HRESULT CDecoder::CodeSpec(ISequentialInStream * inStream,
  ISequentialOutStream * outStream, ICompressProgressInfo * progress)
{
  size_t srcBufLen, result;
  ZSTD_inBuffer zIn;
  ZSTD_outBuffer zOut;

  /* 1) create context */
  if (!_ctx) {
    _ctx = ZSTD_createDCtx();
    if (!_ctx)
      return E_OUTOFMEMORY;

    _srcBuf = MyAlloc(_srcBufSize);
    if (!_srcBuf)
      return E_OUTOFMEMORY;

    _dstBuf = MyAlloc(_dstBufSize);
    if (!_dstBuf)
      return E_OUTOFMEMORY;
  } else {
    ZSTD_resetDStream(_ctx);
  }

  zIn.src = _srcBuf;
  zIn.size = _srcBufSize;
  zIn.pos = 0;

  zOut.dst = _dstBuf;
  srcBufLen = _srcBufSize;

  /* read first input block */
  RINOK(ReadStream(inStream, _srcBuf, &srcBufLen));
  _processedIn += srcBufLen;

  /* Main decompression Loop */
  for (;;) {
    for (;;) {
      /* decompress loop */
      zOut.size = _dstBufSize;
      zOut.pos = 0;

      result = ZSTD_decompressStream(_ctx, &zOut, &zIn);
      if (ZSTD_isError(result)) {
        return E_FAIL;
      }

#if DEBUG
      printf("res       =%u\n", (unsigned)result);
      printf("zIn.size  =%u\n", (unsigned)zIn.size);
      printf("zIn.pos   =%u\n", (unsigned)zIn.pos);
      printf("zOut.size =%u\n", (unsigned)zOut.size);
      printf("zOut.pos  =%u\n", (unsigned)zOut.pos);
      fflush(stdout);
#endif

      /* write decompressed result */
      if (zOut.pos) {
        RINOK(WriteStream(outStream, _dstBuf, zOut.pos));
        _processedOut += zOut.pos;
        RINOK(progress->SetRatioInfo(&_processedIn, &_processedOut));
      }

      /* one more round */
      if ((zIn.pos == zIn.size) && (result == 1) && zOut.pos)
        continue;

      /* finished with buffer */
      if (zIn.pos == zIn.size)
        break;

      /* end of frame */
      if (result == 0) {
        result = ZSTD_resetDStream(_ctx);
        if (ZSTD_isError(result))
          return E_FAIL;
        /* read next input, or eof */
        break;
      }
    } /* for() decompress */

    /* read next input */
    srcBufLen = _srcBufSize;
    RINOK(ReadStream(inStream, _srcBuf, &srcBufLen));
    _processedIn += srcBufLen;

    /* finished */
    if (srcBufLen == 0)
      return S_OK;

    zIn.size = srcBufLen;
    zIn.pos = 0;
  }
}

STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
  const UInt64 * /*inSize */, const UInt64 *outSize, ICompressProgressInfo * progress)
{
  SetOutStreamSize(outSize);
  return CodeSpec(inStream, outStream, progress);
}

#ifndef NO_READ_FROM_CODER
STDMETHODIMP CDecoder::SetInStream(ISequentialInStream * inStream)
{
  _inStream = inStream;
  return S_OK;
}

STDMETHODIMP CDecoder::ReleaseInStream()
{
  _inStream.Release();
  return S_OK;
}
#endif

STDMETHODIMP CDecoder::SetNumberOfThreads(UInt32 /* numThreads */)
{
  return S_OK;
}

HRESULT CDecoder::CodeResume(ISequentialOutStream * outStream, const UInt64 * outSize, ICompressProgressInfo * progress)
{
  RINOK(SetOutStreamSizeResume(outSize));
  return CodeSpec(_inStream, outStream, progress);
}

}}
