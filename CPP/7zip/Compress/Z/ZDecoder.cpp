// ZDecoder.cpp

#include "StdAfx.h"

#include "ZDecoder.h"

extern "C" 
{ 
#include "../../../../C/Alloc.h"
}

#include "../../Common/InBuffer.h"
#include "../../Common/OutBuffer.h"
#include "../../Common/LSBFDecoder.h"

namespace NCompress {
namespace NZ {

static const UInt32 kBufferSize = (1 << 20);
static const Byte kNumBitsMask = 0x1F;   
static const Byte kBlockModeMask = 0x80;   
static const int kNumMinBits = 9;   
static const int kNumMaxBits = 16;   

void CDecoder::Free()
{
  MyFree(_parents);
  _parents = 0;
  MyFree(_suffixes);
  _suffixes = 0;
  MyFree(_stack);
  _stack = 0;
}

bool CDecoder::Alloc(size_t numItems)
{
  Free();
  _parents = (UInt16 *)MyAlloc(numItems * sizeof(UInt16));
  if (_parents == 0)
    return false;
  _suffixes = (Byte *)MyAlloc(numItems * sizeof(Byte));
  if (_suffixes == 0)
    return false;
  _stack = (Byte *)MyAlloc(numItems * sizeof(Byte));
  return _stack != 0;
}

CDecoder::~CDecoder()
{
  Free();
}

STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 * /* inSize */, const UInt64 * /* outSize */,
    ICompressProgressInfo *progress)
{
  NStream::NLSBF::CBaseDecoder<CInBuffer> inBuffer;
  COutBuffer outBuffer;

  if (!inBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;
  inBuffer.SetStream(inStream);
  inBuffer.Init();

  if (!outBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;
  outBuffer.SetStream(outStream);
  outBuffer.Init();

  int maxbits = _properties & kNumBitsMask;
  if (maxbits < kNumMinBits || maxbits > kNumMaxBits)
    return S_FALSE;
  UInt32 numItems = 1 << maxbits;
  bool blockMode = ((_properties & kBlockModeMask) != 0);
  if (!blockMode)
    return E_NOTIMPL;

  if (maxbits != _numMaxBits || _parents == 0 || _suffixes == 0 || _stack == 0)
  {
    if (!Alloc(numItems))
      return E_OUTOFMEMORY;
    _numMaxBits = maxbits;
  }

  UInt64 prevPos = 0;
  int numBits = kNumMinBits;
  UInt32 head = blockMode ? 257 : 256;

  bool needPrev = false;

  int keepBits = 0;

  _parents[256] = 0; // virus protection
  _suffixes[256] = 0;

  for (;;)
  {
    if (keepBits < numBits)
      keepBits = numBits * 8;
    UInt32 symbol = inBuffer.ReadBits(numBits);
    if (inBuffer.ExtraBitsWereRead())
      break;
    keepBits -= numBits;
    if (symbol >= head)
      return S_FALSE;
    if (blockMode && symbol == 256)
    {
      for (;keepBits > 0; keepBits--)
        inBuffer.ReadBits(1);
      numBits = kNumMinBits;
      head = 257;
      needPrev = false;
      continue;
    }
    UInt32 cur = symbol;
    int i = 0;
    while (cur >= 256)
    {
      _stack[i++] = _suffixes[cur];
      cur = _parents[cur];
    }
    _stack[i++] = (Byte)cur;
    if (needPrev)
    {
      _suffixes[head - 1] = (Byte)cur;
      if (symbol == head - 1)
        _stack[0] = (Byte)cur;
    }
    while (i > 0)
      outBuffer.WriteByte((_stack[--i]));
    if (head < numItems)
    {
      needPrev = true;
      _parents[head++] = (UInt16)symbol;
      if (head > ((UInt32)1 << numBits))
      {
        if (numBits < maxbits)
        {
          numBits++;
          keepBits = numBits * 8;
        }
      }
    }
    else
      needPrev = false;

    UInt64 nowPos = outBuffer.GetProcessedSize();
    if (progress != NULL && nowPos - prevPos > (1 << 18))
    {
      prevPos = nowPos;
      UInt64 packSize = inBuffer.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&packSize, &nowPos));
    }
  }
  return outBuffer.Flush();
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size)
{
  if (size < 1)
    return E_INVALIDARG;
  _properties = data[0];
  return S_OK;
}

}}
