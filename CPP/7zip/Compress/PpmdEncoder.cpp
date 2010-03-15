// PpmdEncoder.cpp
// 2009-03-11 : Igor Pavlov : Public domain

#include "StdAfx.h"

#include "../../../C/Alloc.h"
#include "../../../C/CpuArch.h"

#include "../Common/StreamUtils.h"

#include "PpmdEncoder.h"

namespace NCompress {
namespace NPpmd {

static const UInt32 kBufSize = (1 << 20);

static void *SzBigAlloc(void *, size_t size) { return BigAlloc(size); }
static void SzBigFree(void *, void *address) { BigFree(address); }
static ISzAlloc g_BigAlloc = { SzBigAlloc, SzBigFree };

CEncoder::CEncoder():
  _inBuf(NULL),
  _usedMemSize(1 << 24),
  _order(6)
{
  _rangeEnc.Stream = &_outStream.p;
  Ppmd7_Construct(&_ppmd);
}

CEncoder::~CEncoder()
{
  ::MidFree(_inBuf);
  Ppmd7_Free(&_ppmd, &g_BigAlloc);
}

STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps)
{
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = props[i];
    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    UInt32 v = (UInt32)prop.ulVal;
    switch(propIDs[i])
    {
      case NCoderPropID::kUsedMemorySize:
        if (v < (1 << 16) || v > PPMD7_MAX_MEM_SIZE || (v & 3) != 0)
          return E_INVALIDARG;
        _usedMemSize = v;
        break;
      case NCoderPropID::kOrder:
        if (v < 2 || v > 32)
          return E_INVALIDARG;
        _order = (Byte)v;
        break;
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{
  const UInt32 kPropSize = 5;
  Byte props[kPropSize];
  props[0] = _order;
  SetUi32(props + 1, _usedMemSize);
  return WriteStream(outStream, props, kPropSize);
}

HRESULT CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress)
{
  if (!_inBuf)
  {
    _inBuf = (Byte *)::MidAlloc(kBufSize);
    if (!_inBuf)
      return E_OUTOFMEMORY;
  }
  if (!_outStream.Alloc(1 << 20))
    return E_OUTOFMEMORY;
  if (!Ppmd7_Alloc(&_ppmd, _usedMemSize, &g_BigAlloc))
    return E_OUTOFMEMORY;

  _outStream.Stream = outStream;
  _outStream.Init();

  Ppmd7z_RangeEnc_Init(&_rangeEnc);
  Ppmd7_Init(&_ppmd, _order);

  UInt64 processed = 0;
  for (;;)
  {
    UInt32 size;
    RINOK(inStream->Read(_inBuf, kBufSize, &size));
    if (size == 0)
    {
      // We don't write EndMark in PPMD-7z.
      // Ppmd7_EncodeSymbol(&_ppmd, &_rangeEnc, -1);
      Ppmd7z_RangeEnc_FlushData(&_rangeEnc);
      return _outStream.Flush();
    }
    for (UInt32 i = 0; i < size; i++)
    {
      Ppmd7_EncodeSymbol(&_ppmd, &_rangeEnc, _inBuf[i]);
      RINOK(_outStream.Res);
    }
    processed += size;
    if (progress)
    {
      UInt64 outSize = _outStream.GetProcessed();
      RINOK(progress->SetRatioInfo(&processed, &outSize));
    }
  }
}

}}
