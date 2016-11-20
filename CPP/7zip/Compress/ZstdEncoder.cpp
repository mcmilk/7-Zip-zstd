// (C) 2016 Tino Reichardt

#include "StdAfx.h"
#include "ZstdEncoder.h"
#include "ZstdDecoder.h"

#ifndef EXTRACT_ONLY
namespace NCompress {
namespace NZSTD {

CEncoder::CEncoder():
  _processedIn(0),
  _processedOut(0),
  _inputSize(0),
  _ctx(NULL),
  _numThreads(NWindows::NSystem::GetNumberOfProcessors())
{
  _props.clear();
}

CEncoder::~CEncoder()
{
  if (_ctx)
    ZSTDMT_freeCCtx(_ctx);
}

HRESULT CEncoder::ErrorOut(size_t code)
{
  const char *strError = ZSTDMT_getErrorString(code);
  wchar_t wstrError[200+5]; /* no malloc here, /TR */

  mbstowcs(wstrError, strError, 200);
  MessageBoxW(0, wstrError, L"7-Zip ZStandard", MB_ICONERROR | MB_OK);
  MyFree(wstrError);

  return S_FALSE;
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
        Byte mylevel = static_cast < Byte > (ZSTDMT_LEVEL_MAX);
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
  ZSTDMT_RdWr_t rdwr;
  size_t result;
  HRESULT res = S_OK;

  struct ZstdStream Rd;
  Rd.inStream = inStream;
  Rd.outStream = outStream;
  Rd.processedIn = &_processedIn;
  Rd.processedOut = &_processedOut;

  struct ZstdStream Wr;
  if (_processedIn == 0)
    Wr.progress = progress;
  else
    Wr.progress = 0;
  Wr.inStream = inStream;
  Wr.outStream = outStream;
  Wr.processedIn = &_processedIn;
  Wr.processedOut = &_processedOut;

  /* 1) setup read/write functions */
  rdwr.fn_read = ::ZstdRead;
  rdwr.fn_write = ::ZstdWrite;
  rdwr.arg_read = (void *)&Rd;
  rdwr.arg_write = (void *)&Wr;

  /* 2) create compression context, if needed */
  if (!_ctx)
    _ctx = ZSTDMT_createCCtx(_numThreads, _props._level, _inputSize);
  if (!_ctx)
    return S_FALSE;

  /* 3) compress */
  result = ZSTDMT_compressCCtx(_ctx, &rdwr);
  if (ZSTDMT_isError(result)) {
    if (result == (size_t)-ZSTDMT_error_canceled)
      return E_ABORT;
    return ErrorOut(result);
  }

  return res;
}

STDMETHODIMP CEncoder::SetNumberOfThreads(UInt32 numThreads)
{
  const UInt32 kNumThreadsMax = ZSTDMT_THREAD_MAX;
  if (numThreads < 1) numThreads = 1;
  if (numThreads > kNumThreadsMax) numThreads = kNumThreadsMax;
  _numThreads = numThreads;
  return S_OK;
}

}}
#endif
