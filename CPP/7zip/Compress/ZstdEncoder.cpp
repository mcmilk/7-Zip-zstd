// ZstdEncoder.cpp
// (C) 2016 Tino Reichardt

#include "StdAfx.h"
#include "ZstdEncoder.h"

#include <stdio.h>

#ifndef EXTRACT_ONLY
struct MyStream {
	ISequentialInStream *inStream;
	ISequentialOutStream *outStream;
	ICompressProgressInfo *progress;
	UInt64 *processedIn;
	UInt64 *processedOut;
};

int MyRead(void *arg, ZSTDMT_Buffer * in)
{
	struct MyStream *x = (struct MyStream*)arg;
	size_t size = static_cast < size_t > (in->size);
        //_props._level = static_cast < Byte > (prop.ulVal);

	HRESULT res = ReadStream(x->inStream, in->buf, &size);
	if (res != 0)
		return -1;

	*x->processedIn += size;
	x->progress->SetRatioInfo(x->processedIn, x->processedOut);
	in->size = static_cast < int > (size);

	return S_OK;
}

int MyWrite(void *arg, ZSTDMT_Buffer * out)
{
	struct MyStream *x = (struct MyStream*)arg;
	HRESULT res = WriteStream(x->outStream, out->buf, out->size);
	if (res != 0)
		return -1;

	*x->processedOut += out->size;
	x->progress->SetRatioInfo(x->processedIn, x->processedOut);

	return S_OK;
}

namespace NCompress {
namespace NZSTD {

CEncoder::CEncoder():
  _processedIn(0),
  _processedOut(0),
  _inputSize(0),
  _numThreads(1)
{
  _props.clear();
}

CEncoder::~CEncoder()
{
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
        Byte zstd_level = static_cast < Byte > (ZSTD_maxCLevel());
        if (_props._level > zstd_level)
          _props._level = zstd_level;

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

  _processedIn = 0;
  _processedOut = 0;

  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream * outStream)
{
  return WriteStream(outStream, &_props, sizeof (_props));
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
  ISequentialOutStream *outStream, const UInt64 * /* inSize */ ,
  const UInt64 * /* outSize */ , ICompressProgressInfo *progress)
{
	ZSTDMT_RdWr_t rdwr;
	int ret;

	struct MyStream Rd;
	Rd.progress = progress;
	Rd.inStream = inStream;
	Rd.processedIn = &_processedIn;
	Rd.processedOut = &_processedOut;

	struct MyStream Wr;
	Wr.progress = progress;
	Wr.outStream = outStream;
	Wr.processedIn = &_processedIn;
	Wr.processedOut = &_processedOut;

	/* 1) setup read/write functions */
	rdwr.fn_read = ::MyRead;
	rdwr.fn_write = ::MyWrite;
	rdwr.arg_read = (void *)&Rd;
	rdwr.arg_write = (void *)&Wr;

	/* 2) create compression context */
	ZSTDMT_CCtx *ctx = ZSTDMT_createCCtx(_numThreads, _props._level, _inputSize);
	if (!ctx)
	    return S_FALSE;
//		perror_exit("Allocating ctx failed!");

	/* 3) compress */
	ret = ZSTDMT_CompressCCtx(ctx, &rdwr);
	if (ret == -1)
	    return S_FALSE;
//		perror_exit("ZSTDMT_CompressCCtx() failed!");

	/* 4) free resources */
	ZSTDMT_freeCCtx(ctx);
	return S_OK;
}

STDMETHODIMP CEncoder::SetNumberOfThreads(UInt32 numThreads)
{
  const UInt32 kNumThreadsMax = 128;
  if (numThreads < 1) numThreads = 1;
  if (numThreads > kNumThreadsMax) numThreads = kNumThreadsMax;
  _numThreads = numThreads;
  return S_OK;
}

HRESULT CEncoder::ErrorOut(size_t code)
{
  const char *strError = ZSTD_getErrorName(code);
  size_t strErrorLen = strlen(strError) + 1;
  wchar_t *wstrError = (wchar_t *)MyAlloc(sizeof(wchar_t) * strErrorLen);

  if (!wstrError)
    return E_FAIL;

  mbstowcs(wstrError, strError, strErrorLen - 1);
  MessageBoxW(0, wstrError, L"7-Zip ZStandard", MB_ICONERROR | MB_OK);
  MyFree(wstrError);

  return E_FAIL;
}
}}
#endif
