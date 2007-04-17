// PPMDDecoder.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "Windows/Defs.h"

#include "PPMDDecoder.h"

namespace NCompress {
namespace NPPMD {

const int kLenIdFinished = -1;
const int kLenIdNeedInit = -2;

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *properties, UInt32 size)
{
  if (size < 5)
    return E_INVALIDARG;
  _order = properties[0];
  _usedMemorySize = 0;
  for (int i = 0; i < 4; i++)
    _usedMemorySize += ((UInt32)(properties[1 + i])) << (i * 8);

  if (_usedMemorySize > kMaxMemBlockSize)
    return E_NOTIMPL;

  if (!_rangeDecoder.Create(1 << 20))
    return E_OUTOFMEMORY;
  if (!_info.SubAllocator.StartSubAllocator(_usedMemorySize)) 
    return E_OUTOFMEMORY;

  return S_OK;
}

class CDecoderFlusher
{
  CDecoder *_coder;
public:
  bool NeedFlush;
  CDecoderFlusher(CDecoder *coder): _coder(coder), NeedFlush(true) {}
  ~CDecoderFlusher()
  {
    if (NeedFlush)
      _coder->Flush();
    _coder->ReleaseStreams();
  }
};

HRESULT CDecoder::CodeSpec(UInt32 size, Byte *memStream)
{
  if (_outSizeDefined)
  {
    const UInt64 rem = _outSize - _processedSize;
    if (size > rem)
      size = (UInt32)rem;
  }
  const UInt32 startSize = size;

  if (_remainLen == kLenIdFinished)
    return S_OK;
  if (_remainLen == kLenIdNeedInit)
  {
    _rangeDecoder.Init();
    _remainLen = 0;
    _info.MaxOrder = 0;
    _info.StartModelRare(_order);
  }
  while (size != 0)
  {
    int symbol = _info.DecodeSymbol(&_rangeDecoder);
    if (symbol < 0)
    {
      _remainLen = kLenIdFinished;
      break;
    }
    if (memStream != 0)
      *memStream++ = (Byte)symbol;
    else
      _outStream.WriteByte((Byte)symbol);
    size--;
  }
  _processedSize += startSize - size;
  return S_OK;
}

STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 * /* inSize */, const UInt64 *outSize,
      ICompressProgressInfo *progress)
{
  if (!_outStream.Create(1 << 20))
    return E_OUTOFMEMORY;
  
  SetInStream(inStream);
  _outStream.SetStream(outStream);
  SetOutStreamSize(outSize);
  CDecoderFlusher flusher(this);

  for (;;)
  {
    _processedSize = _outStream.GetProcessedSize();
    UInt32 curSize = (1 << 18);
    RINOK(CodeSpec(curSize, NULL));
    if (_remainLen == kLenIdFinished)
      break;
    if (progress != NULL)
    {
      UInt64 inSize = _rangeDecoder.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&inSize, &_processedSize));
    }
    if (_outSizeDefined)
      if (_outStream.GetProcessedSize() >= _outSize)
        break;
  }
  flusher.NeedFlush = false;
  return Flush();
}

#ifdef _NO_EXCEPTIONS

#define PPMD_TRY_BEGIN
#define PPMD_TRY_END

#else

#define PPMD_TRY_BEGIN try { 
#define PPMD_TRY_END } \
  catch(const CInBufferException &e)  { return e.ErrorCode; } \
  catch(const COutBufferException &e)  { return e.ErrorCode; } \
  catch(...) { return S_FALSE; }

#endif


STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  PPMD_TRY_BEGIN
  return CodeReal(inStream, outStream, inSize, outSize, progress);
  PPMD_TRY_END
}

STDMETHODIMP CDecoder::SetInStream(ISequentialInStream *inStream)
{
  _rangeDecoder.SetStream(inStream);
  return S_OK;
}

STDMETHODIMP CDecoder::ReleaseInStream()
{
  _rangeDecoder.ReleaseStream();
  return S_OK;
}

STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 *outSize)
{
  _outSizeDefined = (outSize != NULL);
  if (_outSizeDefined)
    _outSize = *outSize;
  _processedSize = 0;
  _remainLen = kLenIdNeedInit;
  _outStream.Init();
  return S_OK;
}

#ifndef NO_READ_FROM_CODER

STDMETHODIMP CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  PPMD_TRY_BEGIN
  if (processedSize)
    *processedSize = 0;
  const UInt64 startPos = _processedSize;
  RINOK(CodeSpec(size, (Byte *)data));
  if (processedSize)
    *processedSize = (UInt32)(_processedSize - startPos);
  return Flush();
  PPMD_TRY_END
}

#endif

}}
