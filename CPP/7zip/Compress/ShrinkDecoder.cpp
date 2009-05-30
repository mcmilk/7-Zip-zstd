// ShrinkDecoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/InBuffer.h"
#include "../Common/OutBuffer.h"

#include "BitlDecoder.h"
#include "ShrinkDecoder.h"

namespace NCompress {
namespace NShrink {

static const UInt32 kBufferSize = (1 << 20);
static const int kNumMinBits = 9;

HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 * /* outSize */, ICompressProgressInfo *progress)
{
  NBitl::CBaseDecoder<CInBuffer> inBuffer;
  COutBuffer outBuffer;

  if (!inBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;
  inBuffer.SetStream(inStream);
  inBuffer.Init();

  if (!outBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;
  outBuffer.SetStream(outStream);
  outBuffer.Init();

  UInt64 prevPos = 0;
  int numBits = kNumMinBits;
  UInt32 head = 257;
  bool needPrev = false;
  UInt32 lastSymbol = 0;

  int i;
  for (i = 0; i < kNumItems; i++)
    _parents[i] = 0;
  for (i = 0; i < kNumItems; i++)
    _suffixes[i] = 0;
  for (i = 0; i < 257; i++)
    _isFree[i] = false;
  for (; i < kNumItems; i++)
    _isFree[i] = true;

  for (;;)
  {
    UInt32 symbol = inBuffer.ReadBits(numBits);
    if (inBuffer.ExtraBitsWereRead())
      break;
    if (_isFree[symbol])
      return S_FALSE;
    if (symbol == 256)
    {
      UInt32 symbol = inBuffer.ReadBits(numBits);
      if (symbol == 1)
      {
        if (numBits < kNumMaxBits)
          numBits++;
      }
      else if (symbol == 2)
      {
        if (needPrev)
          _isFree[head - 1] = true;
        for (i = 257; i < kNumItems; i++)
          _isParent[i] = false;
        for (i = 257; i < kNumItems; i++)
          if (!_isFree[i])
            _isParent[_parents[i]] = true;
        for (i = 257; i < kNumItems; i++)
          if (!_isParent[i])
            _isFree[i] = true;
        head = 257;
        while (head < kNumItems && !_isFree[head])
          head++;
        if (head < kNumItems)
        {
          needPrev = true;
          _isFree[head] = false;
          _parents[head] = (UInt16)lastSymbol;
          head++;
        }
      }
      else
        return S_FALSE;
      continue;
    }
    UInt32 cur = symbol;
    i = 0;
    int corectionIndex = -1;
    while (cur >= 256)
    {
      if (cur == head - 1)
        corectionIndex = i;
      _stack[i++] = _suffixes[cur];
      cur = _parents[cur];
    }
    _stack[i++] = (Byte)cur;
    if (needPrev)
    {
      _suffixes[head - 1] = (Byte)cur;
      if (corectionIndex >= 0)
        _stack[corectionIndex] = (Byte)cur;
    }
    while (i > 0)
      outBuffer.WriteByte((_stack[--i]));
    while (head < kNumItems && !_isFree[head])
      head++;
    if (head < kNumItems)
    {
      needPrev = true;
      _isFree[head] = false;
      _parents[head] = (UInt16)symbol;
      head++;
    }
    else
      needPrev = false;
    lastSymbol = symbol;

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

STDMETHODIMP CDecoder ::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize,    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

}}
