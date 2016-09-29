// ZstdEncoder.cpp
// (C) 2016 Tino Reichardt

#include "StdAfx.h"
#include "ZstdEncoder.h"

#ifndef EXTRACT_ONLY
namespace NCompress {
namespace NZSTD {

CEncoder::CEncoder():
  _cstream(NULL),
  _buffIn(NULL),
  _buffOut(NULL),
  _buffInSize(0),
  _buffOutSize(0),
  _processedIn(0),
  _processedOut(0)
{
  _props.clear();
}

CEncoder::~CEncoder()
{
  if (_cstream)
    ZSTD_freeCStream(_cstream);

  MyFree(_buffIn);
  MyFree(_buffOut);
}

STDMETHODIMP CEncoder::SetCoderProperties(const PROPID * propIDs, const PROPVARIANT * coderProps, UInt32 numProps)
{
  _props.clear();

  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT & prop = coderProps[i];
    PROPID propID = propIDs[i];
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
  size_t result;

  /* init only once in beginning */
  if (!_cstream) {

    /* allocate stream */
    _cstream = ZSTD_createCStream();
    if (!_cstream)
      return E_OUTOFMEMORY;

    /* allocate buffers */
    _buffInSize = ZSTD_CStreamInSize();
    _buffIn = MyAlloc(_buffInSize);
    if (!_buffIn)
      return E_OUTOFMEMORY;

    _buffOutSize = ZSTD_CStreamOutSize();
    _buffOut = MyAlloc(_buffOutSize);
    if (!_buffOut)
      return E_OUTOFMEMORY;
  }

  /* init or re-init stream */
  result = ZSTD_initCStream(_cstream, _props._level);
  if (ZSTD_isError(result))
    return S_FALSE;

  UInt32 read, toRead = static_cast < UInt32 > (_buffInSize);
  for(;;) {

    /* read input */
    RINOK(inStream->Read(_buffIn, toRead, &read));
    size_t InSize = static_cast < size_t > (read);
    _processedIn += InSize;

    if (InSize == 0) {

      /* @eof */
      ZSTD_outBuffer output = { _buffOut, _buffOutSize, 0 };
      result = ZSTD_endStream(_cstream, &output);
      if (ZSTD_isError(result))
        return S_FALSE;

      if (output.pos) {
        /* write last compressed bytes and update progress */
        RINOK(WriteStream(outStream, _buffOut, output.pos));
        _processedOut += output.pos;
        RINOK(progress->SetRatioInfo(&_processedIn, &_processedOut));
      }

      return S_OK;
    }

    /* compress input */
    ZSTD_inBuffer input = { _buffIn, InSize, 0 };
    while (input.pos < input.size) {
      ZSTD_outBuffer output = { _buffOut, _buffOutSize, 0 };
      result = ZSTD_compressStream(_cstream, &output , &input);
      if (ZSTD_isError(result))
        return S_FALSE;
      /* write compressed stream and update progress */
      RINOK(WriteStream(outStream, _buffOut, output.pos));
      _processedOut += output.pos;
      RINOK(progress->SetRatioInfo(&_processedIn, &_processedOut));
    }
  }
}

}}
#endif
