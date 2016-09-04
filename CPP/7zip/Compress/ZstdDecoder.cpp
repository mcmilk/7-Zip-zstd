// ZstdDecoder.cpp
// (C) 2016 Tino Reichardt

#include "StdAfx.h"
#include "ZstdDecoder.h"

namespace NCompress {
namespace NZSTD {

CDecoder::CDecoder():
  _dstream(NULL),
  _buffIn(NULL),
  _buffOut(NULL),
  _buffInSizeAllocated(0),
  _buffOutSizeAllocated(0),
  _buffInSize(ZSTD_DStreamInSize()),
  _buffOutSize(ZSTD_DStreamOutSize()*4),
  _processedIn(0),
  _processedOut(0),
  _propsWereSet(false)
{
  _props.clear();
}

CDecoder::~CDecoder()
{
  if (_dstream)
    ZSTD_freeDStream(_dstream);

  MyFree(_buffIn);
  MyFree(_buffOut);

  _buffInSizeAllocated = 0;
  _buffOutSizeAllocated = 0;
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte * prop, UInt32 size)
{
  DProps *pProps = (DProps *)prop;

  if (size != sizeof(DProps))
    return E_FAIL;

#ifdef ZSTD_LEGACY_SUPPORT
  /* version 0.x and 1.x are okay */
  if (pProps->_ver_major > 1)
    return E_FAIL;

  /* 0.5, 0.6, 0.7, 0.8 are supported! */
  if (pProps->_ver_major == 0) {
    switch (pProps->_ver_minor) {
    case 5:
      break;
    case 6:
      break;
    case 7:
      break;
    case 8:
      break;
    default:
      return E_FAIL;
  }}
#else
  /* only exact version is okay */
  if (pProps->_ver_major != ZSTD_VERSION_MAJOR)
    return E_FAIL;
  if (pProps->_ver_minor != ZSTD_VERSION_MINOR)
    return E_FAIL;

#endif

  memcpy(&_props, pProps, sizeof (DProps));
  _propsWereSet = true;

  return S_OK;
}

HRESULT CDecoder::CreateDecompressor()
{
  size_t result;

  if (!_propsWereSet)
    return E_FAIL;

  if (!_dstream) {
    _dstream = ZSTD_createDStream();
    if (!_dstream)
      return E_FAIL;
  }

  result = ZSTD_initDStream(_dstream);
  if (ZSTD_isError(result))
    return E_FAIL;

  /* allocate buffers */
  if (_buffInSizeAllocated != _buffInSize)
  {
    if (_buffIn)
      MyFree(_buffIn);
    _buffIn = MyAlloc(_buffInSize);

    if (!_buffIn)
      return E_OUTOFMEMORY;
    _buffInSizeAllocated = _buffInSize;
  }

  if (_buffOutSizeAllocated != _buffOutSize)
  {
    if (_buffOut)
      MyFree(_buffOut);
    _buffOut = MyAlloc(_buffOutSize);

    if (!_buffOut)
      return E_OUTOFMEMORY;
    _buffOutSizeAllocated = _buffOutSize;
  }

  return S_OK;
}

HRESULT CDecoder::SetOutStreamSizeResume(const UInt64 * /*outSize*/)
{
  _processedOut = 0;
  RINOK(CreateDecompressor());

  return S_OK;
}

STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 * outSize)
{
  _processedIn = 0;
  RINOK(SetOutStreamSizeResume(outSize));

  return S_OK;
}

STDMETHODIMP CDecoder::SetInBufSize(UInt32, UInt32 size)
{
  _buffInSize = size;
  return S_OK;
}

STDMETHODIMP CDecoder::SetOutBufSize(UInt32, UInt32 size)
{
  _buffOutSize = size;
  return S_OK;
}

HRESULT CDecoder::CodeSpec(ISequentialInStream * inStream, ISequentialOutStream * outStream, ICompressProgressInfo * progress)
{
  RINOK(CreateDecompressor());

  size_t result;
  UInt32 const toRead = static_cast < const UInt32 > (_buffInSize);
  for(;;) {
    UInt32 read;

    /* read input */
    RINOK(inStream->Read(_buffIn, toRead, &read));
    size_t InSize = static_cast < size_t > (read);
    _processedIn += InSize;

    if (InSize == 0)
      return S_OK;

    /* decompress input */
    ZSTD_inBuffer input = { _buffIn, InSize, 0 };
    for (;;) {
      ZSTD_outBuffer output = { _buffOut, _buffOutSize, 0 };
      result = ZSTD_decompressStream(_dstream, &output , &input);
      if (ZSTD_isError(result))
        return S_FALSE;
      /* write decompressed stream and update progress */
      RINOK(WriteStream(outStream, _buffOut, output.pos));
      _processedOut += output.pos;
      RINOK(progress->SetRatioInfo(&_processedIn, &_processedOut));

      /* one more round */
      if ((input.pos == input.size) && (result == 1))
        continue;

      /* finished */
      if (input.pos == input.size)
        break;
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

STDMETHODIMP CDecoder::Read(void *data, UInt32 /*size*/, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;

  size_t result;

  if (!_dstream)
    if (CreateDecompressor() != S_OK)
      return E_FAIL;

  UInt32 read, toRead = static_cast < UInt32 > (_buffInSize);
  Byte *dataout = static_cast < Byte* > (data);
  for(;;) {
    /* read input */
    RINOK(_inStream->Read(_buffIn, toRead, &read));
    size_t InSize = static_cast < size_t > (read);
    _processedIn += InSize;

    if (InSize == 0) {
      return S_OK;
    }

    /* decompress input */
    ZSTD_inBuffer input = { _buffIn, InSize, 0 };
    for (;;) {
      ZSTD_outBuffer output = { dataout, _buffOutSize, 0 };
      result = ZSTD_decompressStream(_dstream, &output , &input);
      if (ZSTD_isError(result))
        return S_FALSE;

      if (processedSize)
        *processedSize += static_cast < UInt32 > (output.pos);

      dataout += output.pos;

      /* one more round */
      if ((input.pos == input.size) && (result == 1))
        continue;

      /* finished */
      if (input.pos == input.size)
        break;
    }
  }
}

HRESULT CDecoder::CodeResume(ISequentialOutStream * outStream, const UInt64 * outSize, ICompressProgressInfo * progress)
{
  RINOK(SetOutStreamSizeResume(outSize));
  return CodeSpec(_inStream, outStream, progress);
}
#endif

}}
