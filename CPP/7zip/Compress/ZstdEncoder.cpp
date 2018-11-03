// (C) 2016 - 2018 Tino Reichardt

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#endif

#include "StdAfx.h"
#include "ZstdEncoder.h"
#include "ZstdDecoder.h"

#ifndef EXTRACT_ONLY
namespace NCompress {
namespace NZSTD {

CEncoder::CEncoder():
  _ctx(NULL),
  _srcBuf(NULL),
  _dstBuf(NULL),
  _srcBufSize(ZSTD_CStreamInSize()),
  _dstBufSize(ZSTD_CStreamOutSize()),
  _processedIn(0),
  _processedOut(0),
  _numThreads(NWindows::NSystem::GetNumberOfProcessors())
{
  _props.clear();
  _hMutex = CreateMutex(NULL, FALSE, NULL);
}

CEncoder::~CEncoder()
{
  if (_ctx) {
    ZSTD_freeCCtx(_ctx);
    MyFree(_srcBuf);
    MyFree(_dstBuf);
    CloseHandle(_hMutex);
  }
}

STDMETHODIMP CEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * coderProps, UInt32 numProps)
{
  _props.clear();

  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT & prop = coderProps[i];
    PROPID propID = propIDs[i];
    UInt32 v = (UInt32)prop.ulVal;
    switch (propID)
    {
    case NCoderPropID::kLevel:
      {
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;

        /* level 1..22 */
        _props._level = static_cast < Byte > (prop.ulVal);
        Byte mylevel = static_cast < Byte > (ZSTD_LEVEL_MAX);
        if (_props._level > mylevel)
          _props._level = mylevel;

        break;
      }
    case NCoderPropID::kNumThreads:
      {
        SetNumberOfThreads(v);
        break;
      }
    default:
      {
        break;
      }
    }
  }

  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream * outStream)
{
  return WriteStream(outStream, &_props, sizeof (_props));
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
  ISequentialOutStream *outStream, const UInt64 * /*inSize*/ ,
  const UInt64 * /*outSize */, ICompressProgressInfo *progress)
{
  ZSTD_EndDirective ZSTD_todo = ZSTD_e_continue;
  ZSTD_outBuffer outBuff;
  ZSTD_inBuffer inBuff;
  size_t err, srcSize;

  if (!_ctx) {
    _ctx = ZSTD_createCCtx();
    if (!_ctx)
      return E_OUTOFMEMORY;

    _srcBuf = MyAlloc(_srcBufSize);
    if (!_srcBuf)
      return E_OUTOFMEMORY;

    _dstBuf = MyAlloc(_dstBufSize);
    if (!_dstBuf)
      return E_OUTOFMEMORY;

    /* setup level */
    err = ZSTD_CCtx_setParameter(_ctx, ZSTD_p_compressionLevel, _props._level);
    if (ZSTD_isError(err)) return E_FAIL;

    /* setup thread count */
    err = ZSTD_CCtx_setParameter(_ctx, ZSTD_p_nbWorkers, _numThreads);
    if (ZSTD_isError(err)) return E_FAIL;

    /* set the content size flag */
    err = ZSTD_CCtx_setParameter(_ctx, ZSTD_p_contentSizeFlag, 1);
    if (ZSTD_isError(err)) return E_FAIL;

    /* todo: make this optional */
    err = ZSTD_CCtx_setParameter(_ctx, ZSTD_p_enableLongDistanceMatching, 1);
    if (ZSTD_isError(err)) return E_FAIL;
  }

  for (;;) {

    /* read input */
    srcSize = _srcBufSize;
    RINOK(ReadStream(inStream, _srcBuf, &srcSize));

    /* eof */
    if (srcSize == 0)
      ZSTD_todo = ZSTD_e_end;

    /* compress data */
    WaitForSingleObject(_hMutex, INFINITE);
    _processedIn += srcSize;
    ReleaseMutex(_hMutex);

    for (;;) {
      outBuff.dst = _dstBuf;
      outBuff.size = _dstBufSize;
      outBuff.pos = 0;

      if (ZSTD_todo == ZSTD_e_continue) {
        inBuff.src = _srcBuf;
        inBuff.size = srcSize;
        inBuff.pos = 0;
      } else {
        inBuff.src = 0;
        inBuff.size = srcSize;
        inBuff.pos = 0;
      }

      err = ZSTD_compress_generic(_ctx, &outBuff, &inBuff, ZSTD_todo);
      if (ZSTD_isError(err)) return E_FAIL;

#if DEBUG
      printf("err=%u ", (unsigned)err);
      printf("srcSize=%u ", (unsigned)srcSize);
      printf("todo=%u\n", ZSTD_todo);
      printf("inBuff.size=%u ", (unsigned)inBuff.size);
      printf("inBuff.pos=%u\n", (unsigned)inBuff.pos);
      printf("outBuff.size=%u ", (unsigned)outBuff.size);
      printf("outBuff.pos=%u\n\n", (unsigned)outBuff.pos);
      fflush(stdout);
#endif

      /* write output */
      if (outBuff.pos) {
        RINOK(WriteStream(outStream, _dstBuf, outBuff.pos));
        WaitForSingleObject(_hMutex, INFINITE);
        _processedOut += outBuff.pos;
        RINOK(progress->SetRatioInfo(&_processedIn, &_processedOut));
        ReleaseMutex(_hMutex);
      }

      /* done */
      if (ZSTD_todo == ZSTD_e_end && err == 0)
        return S_OK;

      /* need more input */
      if (inBuff.pos == inBuff.size)
        break;
    }
  }
}

STDMETHODIMP CEncoder::SetNumberOfThreads(UInt32 numThreads)
{
  const UInt32 kNumThreadsMax = ZSTD_THREAD_MAX;
  if (numThreads < 1) numThreads = 1;
  if (numThreads > kNumThreadsMax) numThreads = kNumThreadsMax;
  _numThreads = numThreads;
  return S_OK;
}

}}
#endif
