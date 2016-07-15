// ZstdDecoder.cpp
// (C) 2016 Rich Geldreich, Tino Reichardt

#include "StdAfx.h"
#include "ZstdDecoder.h"

// #define SHOW_DEBUG_INFO
#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#define PRF(x) x
#else
#define PRF(x)
#endif

#define ZSTD_DEFAULT_BUFFER_SIZE (1U << 22U)

namespace NCompress {
namespace NZSTD {

CDecoder::CDecoder ():
  _inBuf (NULL),
  _outBuf (NULL),
  _inPos (0),
  _inSize (0),
  _eofFlag (false),
  _state (NULL),
  _propsWereSet (false),
  _outSizeDefined (false),
  _outSize (0),

  _inSizeProcessed (0),
  _outSizeProcessed (0),
  _inBufSizeAllocated (0),
  _outBufSizeAllocated (0),

  _inBufSize (ZSTD_DEFAULT_BUFFER_SIZE),
  _outBufSize (ZSTD_DEFAULT_BUFFER_SIZE)
{
  _props.clear ();
}

CDecoder::~CDecoder ()
{
  if (_state)
    ZB_freeDCtx(_state);

  MyFree (_inBuf);
  MyFree (_outBuf);
}

STDMETHODIMP CDecoder::SetInBufSize (UInt32, UInt32 size)
{
  _inBufSize = size;
  return S_OK;
}

STDMETHODIMP CDecoder::SetOutBufSize (UInt32, UInt32 size)
{
  _outBufSize = size;
  return S_OK;
}

HRESULT CDecoder::CreateBuffers ()
{
  if (_inBuf == 0 || _inBufSize != _inBufSizeAllocated)
  {
    MyFree (_inBuf);
    _inBuf = (Byte *) MyAlloc (_inBufSize);
    if (_inBuf == 0)
      return E_OUTOFMEMORY;
    _inBufSizeAllocated = _inBufSize;
  }

  if (_outBuf == 0 || _outBufSize != _outBufSizeAllocated)
  {
    MyFree (_outBuf);
    _outBuf = (Byte *) MyAlloc (_outBufSize);
    if (_outBuf == 0)
      return E_OUTOFMEMORY;
    _outBufSizeAllocated = _outBufSize;
  }

  return S_OK;
}

STDMETHODIMP CDecoder::SetDecoderProperties2 (const Byte * prop, UInt32 size)
{
  DProps *pProps = (DProps *) prop;

  if (size != sizeof (DProps))
    return E_FAIL;

  // version 0.x currently
  if (pProps->_ver_major != ZSTD_VERSION_MAJOR)
    return E_FAIL;

  switch (pProps->_ver_minor) {
  case ZSTD_VERSION_MINOR:
    break;
  case 6:
    break;
  case 5:
    break;
  default:
    return E_FAIL;
  }

  memcpy(&_props, pProps, sizeof (DProps));
  _propsWereSet = true;

  return CreateBuffers();
}

HRESULT CDecoder::CreateDecompressor()
{
  if (!_propsWereSet)
    return E_FAIL;

  if (!_state) {
    _state = ZB_createDCtx();
    if (!_state)
      return E_FAIL;
  }

  if (ZBUFF_isError(ZB_decompressInit(_state)))
    return E_FAIL;

  _eofFlag = false;

  return S_OK;
}

HRESULT CDecoder::SetOutStreamSizeResume(const UInt64 * outSize)
{
  _outSizeDefined = (outSize != NULL);
  if (_outSizeDefined)
    _outSize = *outSize;
  _outSizeProcessed = 0;

  RINOK (CreateDecompressor());

  return S_OK;
}

STDMETHODIMP CDecoder::SetOutStreamSize (const UInt64 * outSize)
{
  _inSizeProcessed = 0;
  _inPos = _inSize = 0;
  RINOK (SetOutStreamSizeResume (outSize));
  return S_OK;
}

HRESULT CDecoder::CodeSpec (ISequentialInStream * inStream,
		      ISequentialOutStream * outStream,
		      ICompressProgressInfo * progress)
{
  if (_inBuf == 0 || !_propsWereSet)
    return S_FALSE;

  if (!_state)
  {
    if (CreateDecompressor () != S_OK)
      return E_FAIL;
  }

  UInt64 startInProgress = _inSizeProcessed;

  for (;;)
  {
    if ((!_eofFlag) && (_inPos == _inSize))
    {
      _inPos = _inSize = 0;
      RINOK (inStream->Read (_inBuf, _inBufSizeAllocated, &_inSize));
      if (!_inSize)
	_eofFlag = true;
    }

    Byte *pIn_bytes = _inBuf + _inPos;
    size_t num_in_bytes = _inSize - _inPos;
    Byte *pOut_bytes = _outBuf;
    size_t num_out_bytes = _outBufSize;
    if (_outSizeDefined)
    {
      UInt64 out_remaining = _outSize - _outSizeProcessed;
      if (out_remaining == 0)
	return S_OK;
      if (num_out_bytes > out_remaining)
	num_out_bytes = static_cast < size_t > (out_remaining);
    }

    size_t decomp_status =
      ZB_decompressContinue (_state, pOut_bytes, &num_out_bytes,
				pIn_bytes, &num_in_bytes);
    bool decomp_finished = (decomp_status == 0);
    bool decomp_failed = ZBUFF_isError (decomp_status) != 0;

    if (num_in_bytes)
    {
      _inPos += (UInt32) num_in_bytes;
      _inSizeProcessed += (UInt32) num_in_bytes;
    }

    if (num_out_bytes)
    {
      _outSizeProcessed += num_out_bytes;

      RINOK (WriteStream (outStream, _outBuf, num_out_bytes));
    }

    if (decomp_failed)
    {
      PRF(fprintf(stderr, "zstdcodec: ZB_decompressContinue() failed: %s\n", ZBUFF_getErrorName(decomp_status)));
      return S_FALSE;
    }

    if (decomp_finished)
      break;

    // This check is to prevent locking up if the input file is accidently truncated.
    bool made_forward_progress = (num_out_bytes != 0) || (num_in_bytes != 0);
    if ((!made_forward_progress) && (_eofFlag))
    {
      return S_FALSE;
    }

    UInt64 inSize = _inSizeProcessed - startInProgress;
    if (progress)
    {
      RINOK (progress->SetRatioInfo (&inSize, &_outSizeProcessed));
    }
  }

  return S_OK;
}

STDMETHODIMP CDecoder::Code (ISequentialInStream * inStream,
		  ISequentialOutStream * outStream,
		  const UInt64 * inSize,
		  const UInt64 * outSize, ICompressProgressInfo * progress)
{
  (void) inSize;
  if (_inBuf == 0)
    return E_INVALIDARG;
  SetOutStreamSize (outSize);
  return CodeSpec (inStream, outStream, progress);
}

// wrapper for different versions
void *CDecoder::ZB_createDCtx(void)
{
  PRF(fprintf(stderr, "zstdcodec: ZB_createDCtx(v=%d)\n", _props._ver_minor));
#ifndef EXTRACT_ONLY
  switch (_props._ver_minor) {
  case 5:
    return (void*)ZBUFFv05_createDCtx();
    break;
  case 6:
    return (void*)ZBUFFv06_createDCtx();
    break;
  }
#endif
  return (void*)ZBUFF_createDCtx();
}

size_t CDecoder::ZB_freeDCtx(void *dctx)
{
  PRF(fprintf(stderr, "zstdcodec: ZB_freeDCtx(v=%d)\n", _props._ver_minor));
#ifndef EXTRACT_ONLY
  switch (_props._ver_minor) {
  case 5:
    return ZBUFFv05_freeDCtx((ZBUFFv05_DCtx *)dctx);
    break;
  case 6:
    return ZBUFFv06_freeDCtx((ZBUFFv06_DCtx *)dctx);
    break;
  }
#endif
  return ZBUFF_freeDCtx((ZBUFF_DCtx *)dctx);
}

size_t CDecoder::ZB_decompressInit(void *dctx)
{
  PRF(fprintf(stderr, "zstdcodec: ZB_decompressInit(v=%d)\n", _props._ver_minor));
#ifndef EXTRACT_ONLY
  switch (_props._ver_minor) {
  case 5:
    return ZBUFFv05_decompressInit((ZBUFFv05_DCtx *)dctx);
    break;
  case 6:
    return ZBUFFv06_decompressInit((ZBUFFv06_DCtx *)dctx);
    break;
  }
#endif
  return ZBUFF_decompressInit((ZBUFF_DCtx *)dctx);
}

size_t CDecoder::ZB_decompressContinue(void *dctx, void* dst, size_t *dstCapacityPtr, const void* src, size_t *srcSizePtr)
{
  PRF(fprintf(stderr, "zstdcodec: ZB_decompressContinue(v=%d)\n", _props._ver_minor));
#ifndef EXTRACT_ONLY
  switch (_props._ver_minor) {
  case 5:
    return ZBUFFv05_decompressContinue((ZBUFFv05_DCtx *)dctx, dst, dstCapacityPtr, src, srcSizePtr);
    break;
  case 6:
    return ZBUFFv06_decompressContinue((ZBUFFv06_DCtx *)dctx, dst, dstCapacityPtr, src, srcSizePtr);
    break;
  }
#endif
  return ZBUFF_decompressContinue((ZBUFF_DCtx *)dctx, dst, dstCapacityPtr, src, srcSizePtr);
}

#ifndef NO_READ_FROM_CODER
STDMETHODIMP CDecoder::SetInStream (ISequentialInStream * inStream)
{
  _inStream = inStream;
  return S_OK;
}

STDMETHODIMP CDecoder::ReleaseInStream ()
{
  _inStream.Release ();
  return S_OK;
}

STDMETHODIMP CDecoder::Read (void *data, UInt32 size, UInt32 * processedSize)
{
  if (processedSize)
    *processedSize = 0;

  if (_inBuf == 0 || !_propsWereSet)
    return S_FALSE;

  if (!_state)
  {
    if (CreateDecompressor () != S_OK)
      return E_FAIL;
  }

  while (size != 0)
  {
    if ((!_eofFlag) && (_inPos == _inSize))
    {
      _inPos = _inSize = 0;
      RINOK (_inStream->Read (_inBuf, _inBufSizeAllocated, &_inSize));
      if (!_inSize)
	_eofFlag = true;
    }

    Byte *pIn_bytes = _inBuf + _inPos;
    size_t  num_in_bytes = _inSize - _inPos;
    Byte *pOut_bytes = (Byte *) data;
    size_t  num_out_bytes = size;

    size_t decomp_status =
      ZB_decompressContinue (_state, pOut_bytes, &num_out_bytes,
				pIn_bytes, &num_in_bytes);
    bool
      decomp_finished = (decomp_status == 0);
    bool
      decomp_failed = ZBUFF_isError (decomp_status) != 0;

    if (num_in_bytes)
    {
      _inPos += (UInt32) num_in_bytes;
      _inSizeProcessed += num_in_bytes;
    }

    if (num_out_bytes)
    {
      _outSizeProcessed += num_out_bytes;
      size -= (UInt32) num_out_bytes;
      if (processedSize)
	*processedSize += (UInt32) num_out_bytes;
    }

    if (decomp_failed)
    {
      return S_FALSE;
    }

    if (decomp_finished)
      break;

    // This check is to prevent locking up if the input file is accidently truncated.
    bool made_forward_progress = (num_out_bytes != 0) || (num_in_bytes != 0);
    if ((!made_forward_progress) && (_eofFlag))
    {
      return S_FALSE;
    }
  }

  return S_OK;
}

HRESULT CDecoder::CodeResume (ISequentialOutStream * outStream,
			const UInt64 * outSize,
			ICompressProgressInfo * progress)
{
  RINOK (SetOutStreamSizeResume (outSize));
  return CodeSpec (_inStream, outStream, progress);
}
#endif

}}
