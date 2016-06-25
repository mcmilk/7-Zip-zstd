// ZstdEncoder.cpp
// (C) 2016 Rich Geldreich, Tino Reichardt

#include "StdAfx.h"
#include "ZstdEncoder.h"

// #define SHOW_DEBUG_INFO
#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#define PRF(x) x
#else
#define PRF(x)
#endif

#define ZSTD_DEFAULT_BUFFER_SIZE (1U << 22U)

#ifndef EXTRACT_ONLY
namespace NCompress {
namespace NZSTD {

CEncoder::CEncoder():
  _state (NULL),
  _inBuf (NULL),
  _outBuf (NULL),
  _inPos (0),
  _inSize (0),
  _inBufSizeAllocated (0),
  _outBufSizeAllocated (0),
  _inBufSize (ZSTD_DEFAULT_BUFFER_SIZE),
  _outBufSize (ZSTD_DEFAULT_BUFFER_SIZE),
  _inSizeProcessed (0),
  _outSizeProcessed (0)
{
  _props.clear ();
}

CEncoder::~CEncoder ()
{
  if (_state)
    ZBUFF_freeCCtx(_state);

  MyFree (_inBuf);
  MyFree (_outBuf);
}

STDMETHODIMP CEncoder::SetCoderProperties (const PROPID * propIDs,
  const PROPVARIANT * coderProps, UInt32 numProps)
{
  _props.clear ();

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

  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties (ISequentialOutStream * outStream)
{
  return WriteStream (outStream, &_props, sizeof (_props));
}

HRESULT CEncoder::CreateBuffers ()
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

HRESULT CEncoder::CreateCompressor ()
{
  if (!_state) {
    _state = ZBUFF_createCCtx();
    if (!_state)
      return S_FALSE;
  }

  if (ZBUFF_compressInit(_state, _props._level))
    return S_FALSE;

  return S_OK;
}

STDMETHODIMP CEncoder::Code (ISequentialInStream * inStream,
  ISequentialOutStream * outStream, const UInt64 * /* inSize */ ,
  const UInt64 * /* outSize */ , ICompressProgressInfo * progress)
{
  RINOK (CreateCompressor());
  RINOK (CreateBuffers());

  UInt64 startInProgress = _inSizeProcessed;
  UInt64 startOutProgress = _outSizeProcessed;

  for (;;)
  {
    bool eofFlag = false;
    if (_inPos == _inSize)
    {
      _inPos = _inSize = 0;
      RINOK (inStream->Read (_inBuf, _inBufSizeAllocated, &_inSize));
      if (!_inSize)
	eofFlag = true;
    }

    Byte *pIn_bytes = _inBuf + _inPos;
    size_t num_in_bytes = _inSize - _inPos;
    Byte *pOut_bytes = _outBuf;
    size_t num_out_bytes = _outBufSize;

    size_t comp_status;
    bool comp_finished = false;

    if (eofFlag)
    {
      comp_status = ZBUFF_compressEnd (_state, pOut_bytes, &num_out_bytes);
      comp_finished = (comp_status == 0);
    }
    else
    {
      comp_status = ZBUFF_compressContinue(_state, pOut_bytes, &num_out_bytes, pIn_bytes, &num_in_bytes);
    }

    bool comp_failed = ZBUFF_isError (comp_status) != 0;

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

    if (comp_failed)
      return S_FALSE;

    if (comp_finished)
      break;

    UInt64 inSize = _inSizeProcessed - startInProgress;
    UInt64 outSize = _outSizeProcessed - startOutProgress;
    if (progress)
    {
      RINOK (progress->SetRatioInfo (&inSize, &outSize));
    }
  }

  return S_OK;
}

}}
#endif
