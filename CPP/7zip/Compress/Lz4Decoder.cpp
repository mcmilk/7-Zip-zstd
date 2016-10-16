// (C) 2016 Tino Reichardt

#include "StdAfx.h"
#include "Lz4Decoder.h"

int Lz4Read(void *arg, LZ4MT_Buffer * in)
{
  struct Lz4Stream *x = (struct Lz4Stream*)arg;
  size_t size = in->size;

  HRESULT res = ReadStream(x->inStream, in->buf, &size);
  if (res != S_OK)
    return -1;

  in->size = size;
  *x->processedIn += size;

  return 0;
}

int Lz4Write(void *arg, LZ4MT_Buffer * out)
{
  struct Lz4Stream *x = (struct Lz4Stream*)arg;
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
namespace NLZ4 {

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
  const char *strError = LZ4MT_getErrorString(code);
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
  const UInt32 kNumThreadsMax = LZ4MT_THREAD_MAX;
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
  LZ4MT_RdWr_t rdwr;
  size_t result;
  HRESULT res = S_OK;

  struct Lz4Stream Rd;
  Rd.inStream = inStream;
  Rd.processedIn = &_processedIn;

  struct Lz4Stream Wr;
  Wr.progress = progress;
  Wr.outStream = outStream;
  Wr.processedIn = &_processedIn;
  Wr.processedOut = &_processedOut;

  /* 1) setup read/write functions */
  rdwr.fn_read = ::Lz4Read;
  rdwr.fn_write = ::Lz4Write;
  rdwr.arg_read = (void *)&Rd;
  rdwr.arg_write = (void *)&Wr;

  /* 2) create compression context */
  LZ4MT_DCtx *ctx = LZ4MT_createDCtx(_numThreads, _inputSize);
  if (!ctx)
      return S_FALSE;

  /* 3) compress */
  result = LZ4MT_DecompressDCtx(ctx, &rdwr);
  if (result == (size_t)-LZ4MT_error_read_fail)
    res = E_ABORT;
  else if (LZ4MT_isError(result))
    return ErrorOut(result);

  /* 4) free resources */
  LZ4MT_freeDCtx(ctx);
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
