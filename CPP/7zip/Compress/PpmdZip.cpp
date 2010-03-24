// PpmdZip.cpp
// 2010-03-24 : Igor Pavlov : Public domain

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "../Common/StreamUtils.h"

#include "PpmdZip.h"

namespace NCompress {
namespace NPpmdZip {

static void *SzBigAlloc(void *, size_t size) { return BigAlloc(size); }
static void SzBigFree(void *, void *address) { BigFree(address); }
static ISzAlloc g_BigAlloc = { SzBigAlloc, SzBigFree };

CDecoder::CDecoder(bool fullFileMode):
  _fullFileMode(fullFileMode)
{
  _ppmd.Stream.In = &_inStream.p;
  Ppmd8_Construct(&_ppmd);
}

CDecoder::~CDecoder()
{
  Ppmd8_Free(&_ppmd, &g_BigAlloc);
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  if (!_outStream.Alloc())
    return E_OUTOFMEMORY;
  if (!_inStream.Alloc(1 << 20))
    return E_OUTOFMEMORY;

  _inStream.Stream = inStream;
  _inStream.Init();

  {
    Byte buf[2];
    for (int i = 0; i < 2; i++)
      buf[i] = _inStream.ReadByte();
    if (_inStream.Extra)
      return S_FALSE;
    
    UInt32 val = GetUi16(buf);
    UInt32 order = (val & 0xF) + 1;
    UInt32 mem = ((val >> 4) & 0xFF) + 1;
    UInt32 restor = (val >> 12);
    if (order < 2 || restor > 2)
      return S_FALSE;
    
    #ifndef PPMD8_FREEZE_SUPPORT
    if (restor == 2)
      return E_NOTIMPL;
    #endif
    
    if (!Ppmd8_Alloc(&_ppmd, mem << 20, &g_BigAlloc))
      return E_OUTOFMEMORY;
    
    if (!Ppmd8_RangeDec_Init(&_ppmd))
      return S_FALSE;
    Ppmd8_Init(&_ppmd, order, restor);
  }

  bool wasFinished = false;
  UInt64 processedSize = 0;
  while (!outSize || processedSize < *outSize)
  {
    size_t size = kBufSize;
    if (outSize != NULL)
    {
      const UInt64 rem = *outSize - processedSize;
      if (size > rem)
        size = (size_t)rem;
    }
    Byte *data = _outStream.Buf;
    size_t i = 0;
    int sym = 0;
    do
    {
      sym = Ppmd8_DecodeSymbol(&_ppmd);
      if (_inStream.Extra || sym < 0)
        break;
      data[i] = (Byte)sym;
    }
    while (++i != size);
    processedSize += i;

    RINOK(WriteStream(outStream, _outStream.Buf, i));

    RINOK(_inStream.Res);
    if (_inStream.Extra)
      return S_FALSE;

    if (sym < 0)
    {
      if (sym != -1)
        return S_FALSE;
      wasFinished = true;
      break;
    }
    if (progress)
    {
      UInt64 inSize = _inStream.GetProcessed();
      RINOK(progress->SetRatioInfo(&inSize, &processedSize));
    }
  }
  RINOK(_inStream.Res);
  if (_fullFileMode)
  {
    if (!wasFinished)
    {
      int res = Ppmd8_DecodeSymbol(&_ppmd);
      RINOK(_inStream.Res);
      if (_inStream.Extra || res != -1)
        return S_FALSE;
    }
    if (!Ppmd8_RangeDec_IsFinishedOK(&_ppmd))
      return S_FALSE;
  }
  return S_OK;
}


// ---------- Encoder ----------

CEncoder::~CEncoder()
{
  Ppmd8_Free(&_ppmd, &g_BigAlloc);
}

HRESULT CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps)
{
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = props[i];
    if (prop.vt != VT_UI4)
      return E_INVALIDARG;
    UInt32 v = (UInt32)prop.ulVal;
    switch(propIDs[i])
    {
      case NCoderPropID::kAlgorithm:
        if (v > 1)
          return E_INVALIDARG;
        _restor = v;
        break;
      case NCoderPropID::kUsedMemorySize:
        if (v < (1 << 20) || v > (1 << 28))
          return E_INVALIDARG;
        _usedMemInMB = v >> 20;
        break;
      case NCoderPropID::kOrder:
        if (v < PPMD8_MIN_ORDER || v > PPMD8_MAX_ORDER)
          return E_INVALIDARG;
        _order = (Byte)v;
        break;
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

CEncoder::CEncoder():
  _usedMemInMB(16),
  _order(6),
  _restor(PPMD8_RESTORE_METHOD_RESTART)
{
  _ppmd.Stream.Out = &_outStream.p;
  Ppmd8_Construct(&_ppmd);
}

HRESULT CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress)
{
  if (!_inStream.Alloc())
    return E_OUTOFMEMORY;
  if (!_outStream.Alloc(1 << 20))
    return E_OUTOFMEMORY;
  if (!Ppmd8_Alloc(&_ppmd, _usedMemInMB << 20, &g_BigAlloc))
    return E_OUTOFMEMORY;

  _outStream.Stream = outStream;
  _outStream.Init();

  Ppmd8_RangeEnc_Init(&_ppmd);
  Ppmd8_Init(&_ppmd, _order, _restor);

  UInt32 val = (UInt32)((_order - 1) + ((_usedMemInMB - 1) << 4) + (_restor << 12));
  _outStream.WriteByte((Byte)(val & 0xFF));
  _outStream.WriteByte((Byte)(val >> 8));
  RINOK(_outStream.Res);

  UInt64 processed = 0;
  for (;;)
  {
    UInt32 size;
    RINOK(inStream->Read(_inStream.Buf, kBufSize, &size));
    if (size == 0)
    {
      Ppmd8_EncodeSymbol(&_ppmd, -1);
      Ppmd8_RangeEnc_FlushData(&_ppmd);
      return _outStream.Flush();
    }
    for (UInt32 i = 0; i < size; i++)
    {
      Ppmd8_EncodeSymbol(&_ppmd, _inStream.Buf[i]);
      RINOK(_outStream.Res);
    }
    processed += size;
    if (progress != NULL)
    {
      UInt64 outSize = _outStream.GetProcessed();
      RINOK(progress->SetRatioInfo(&processed, &outSize));
    }
  }
}

}}
