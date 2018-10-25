// (C) 2016 - 2018 Tino Reichardt

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
  _processedOut(0),
  _numThreads(NWindows::NSystem::GetNumberOfProcessors())

{
  _props.clear();
  _hMutex = CreateMutex(NULL, FALSE, NULL);
}

CDecoder::~CDecoder()
{
  if (_ctx) {
    ZSTD_freeDCtx(_ctx);
    MyFree(_srcBuf);
    MyFree(_dstBuf);
    CloseHandle(_hMutex);
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

STDMETHODIMP CDecoder::SetNumberOfThreads(UInt32 numThreads)
{
  const UInt32 kNumThreadsMax = ZSTD_THREAD_MAX;
  if (numThreads < 1) numThreads = 1;
  if (numThreads > kNumThreadsMax) numThreads = kNumThreadsMax;
  _numThreads = numThreads;
  return S_OK;
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

  for (;;) {
    size_t size = _srcBufSize;
    RINOK(ReadStream(inStream, _srcBuf, &size));
    for (;;) {
      ZSTD_inBuffer  inBuff = { _srcBuf, size, 0 };
      ZSTD_outBuffer outBuff= { _dstBuf, _dstBufSize, 0 };
      size_t const readSizeHint = ZSTD_decompressStream(_ctx, &outBuff, &inBuff);

      if (ZSTD_isError(readSizeHint))
          return E_FAIL;

      /* write decompressed data */
      RINOK(WriteStream(outStream, _dstBuf, outBuff.pos));
      WaitForSingleObject(_hMutex, INFINITE);
      _processedOut += outBuff.pos;
      RINOK(progress->SetRatioInfo(&_processedIn, &_processedOut));
      ReleaseMutex(_hMutex);

      if (inBuff.pos > 0) {
        memmove(_srcBuf, (char*)_srcBuf + inBuff.pos, inBuff.size - inBuff.pos);
        size = _srcBufSize - inBuff.pos;
        RINOK(ReadStream(inStream, (char*)_srcBuf + inBuff.pos, &size));
      }

      if (inBuff.size != inBuff.pos)
          return E_FAIL;

      /* eof */
      if (readSizeHint == 0)
        return S_OK;
    }
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

HRESULT CDecoder::CodeResume(ISequentialOutStream * outStream, const UInt64 * outSize, ICompressProgressInfo * progress)
{
  RINOK(SetOutStreamSizeResume(outSize));
  return CodeSpec(_inStream, outStream, progress);
}

}}
