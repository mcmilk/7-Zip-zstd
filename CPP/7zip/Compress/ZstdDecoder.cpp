// (C) 2016 Tino Reichardt

#include "StdAfx.h"
#include "ZstdDecoder.h"

int ZstdRead(void *arg, ZSTDMT_Buffer * in)
{
  struct ZstdStream *x = (struct ZstdStream*)arg;
  size_t size = in->size;

  HRESULT res = ReadStream(x->inStream, in->buf, &size);
  if (res != S_OK)
    return -1;

  in->size = size;
  *x->processedIn += size;

  return 0;
}

int ZstdWrite(void *arg, ZSTDMT_Buffer * out)
{
  struct ZstdStream *x = (struct ZstdStream*)arg;
  UInt32 todo = (UInt32)out->size;
  UInt32 done = 0;

  while (todo != 0)
  {
    UInt32 block;
    HRESULT res = x->outStream->Write((char*)out->buf + done, todo, &block);
    done += block;
    if (res == k_My_HRESULT_WritingWasCut)
      break;
    if (res != S_OK)
      return -1;
    if (block == 0)
      return E_FAIL;
    todo -= block;
  }

  *x->processedOut += done;
  if (x->progress)
    x->progress->SetRatioInfo(x->processedIn, x->processedOut);

  return 0;
}

namespace NCompress {
namespace NZSTD {

CDecoder::CDecoder():
  _processedIn(0),
  _processedOut(0),
  _inputSize(0),
  _numThreads(NWindows::NSystem::GetNumberOfProcessors())
{
  _props.clear();
}

CDecoder::~CDecoder()
{
}

HRESULT CDecoder::ErrorOut(size_t code)
{
  const char *strError = ZSTDMT_getErrorString(code);
  wchar_t wstrError[200+5]; /* no malloc here, /TR */

  mbstowcs(wstrError, strError, 200);
  MessageBoxW(0, wstrError, L"7-Zip ZStandard", MB_ICONERROR | MB_OK);
  MyFree(wstrError);

  return S_FALSE;
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * prop, UInt32 size)
{
  DProps *pProps = (DProps *)prop;

  if (size != sizeof(DProps))
    return E_FAIL;

  memcpy(&_props, pProps, sizeof (DProps));

  return S_OK;
}

STDMETHODIMP CDecoder::SetNumberOfThreads(UInt32 numThreads)
{
  const UInt32 kNumThreadsMax = ZSTDMT_THREAD_MAX;
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
  ZSTDMT_RdWr_t rdwr;
  size_t result;
  HRESULT res = S_OK;

  struct ZstdStream Rd;
  Rd.inStream = inStream;
  Rd.processedIn = &_processedIn;

  struct ZstdStream Wr;
  Wr.progress = progress;
  Wr.outStream = outStream;
  Wr.processedIn = &_processedIn;
  Wr.processedOut = &_processedOut;

  /* 1) setup read/write functions */
  rdwr.fn_read = ::ZstdRead;
  rdwr.fn_write = ::ZstdWrite;
  rdwr.arg_read = (void *)&Rd;
  rdwr.arg_write = (void *)&Wr;

  /* 2) create decompression context */
  ZSTDMT_DCtx *ctx = ZSTDMT_createDCtx(_numThreads, _inputSize);
  if (!ctx)
      return S_FALSE;

  /* 3) decompress */
  result = ZSTDMT_decompressDCtx(ctx, &rdwr);
  //printf("decompress = %d / %d\n", result, ZSTDMT_error_read_fail);
  if (result == (size_t)-ZSTDMT_error_read_fail)
    res = E_ABORT;
  else if (ZSTDMT_isError(result))
    return ErrorOut(result);

  /* 4) free resources */
  ZSTDMT_freeDCtx(ctx);
  return res;
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
