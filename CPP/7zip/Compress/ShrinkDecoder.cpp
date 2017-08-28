// ShrinkDecoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/InBuffer.h"
#include "../Common/OutBuffer.h"

#include "BitlDecoder.h"
#include "ShrinkDecoder.h"

namespace NCompress {
namespace NShrink {

static const UInt32 kBufferSize = (1 << 18);
static const unsigned kNumMinBits = 9;

HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  NBitl::CBaseDecoder<CInBuffer> inBuffer;
  COutBuffer outBuffer;

  if (!inBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;
  if (!outBuffer.Create(kBufferSize))
    return E_OUTOFMEMORY;

  inBuffer.SetStream(inStream);
  inBuffer.Init();

  outBuffer.SetStream(outStream);
  outBuffer.Init();

  {
    unsigned i;
    for (i = 0; i < 257; i++)
      _parents[i] = (UInt16)i;
    for (; i < kNumItems; i++)
      _parents[i] = kNumItems;
    for (i = 0; i < kNumItems; i++)
      _suffixes[i] = 0;
  }

  UInt64 prevPos = 0, inPrev = 0;
  unsigned numBits = kNumMinBits;
  unsigned head = 257;
  int lastSym = -1;
  Byte lastChar2 = 0;
  bool moreOut = false;

  HRESULT res = S_FALSE;

  for (;;)
  {
    _inProcessed = inBuffer.GetProcessedSize();
    const UInt64 nowPos = outBuffer.GetProcessedSize();

    bool eofCheck = false;

    if (outSize && nowPos >= *outSize)
    {
      if (!_fullStreamMode || moreOut)
      {
        res = S_OK;
        break;
      }
      eofCheck = true;
      // Is specSym(=256) allowed after end of stream
      // Do we need to read it here
    }

    if (progress)
    {
      if (nowPos - prevPos >= (1 << 18)
          || _inProcessed - inPrev >= (1 << 20))
      {
        prevPos = nowPos;
        inPrev = _inProcessed;
        RINOK(progress->SetRatioInfo(&_inProcessed, &nowPos));
      }
    }

    UInt32 sym = inBuffer.ReadBits(numBits);

    if (inBuffer.ExtraBitsWereRead())
    {
      res = S_OK;
      break;
    }
    
    if (sym == 256)
    {
      sym = inBuffer.ReadBits(numBits);

      if (inBuffer.ExtraBitsWereRead())
        break;

      if (sym == 1)
      {
        if (numBits >= kNumMaxBits)
          break;
        numBits++;
        continue;
      }
      if (sym != 2)
        break;
      {
        unsigned i;
        for (i = 257; i < kNumItems; i++)
          _stack[i] = 0;
        for (i = 257; i < kNumItems; i++)
        {
          unsigned par = _parents[i];
          if (par != kNumItems)
            _stack[par] = 1;
        }
        for (i = 257; i < kNumItems; i++)
          if (_stack[i] == 0)
            _parents[i] = kNumItems;
       
        head = 257;
       
        continue;
      }
    }

    if (eofCheck)
    {
      // It's can be error case.
      // That error can be detected later in (*inSize != _inProcessed) check.
      res = S_OK;
      break;
    }

    bool needPrev = false;
    if (head < kNumItems && lastSym >= 0)
    {
      while (head < kNumItems && _parents[head] != kNumItems)
        head++;
      if (head < kNumItems)
      {
        if (head == (unsigned)lastSym)
        {
          // we need to fix the code for that case
          // _parents[head] is not allowed to link to itself
          res = E_NOTIMPL;
          break;
        }
        needPrev = true;
        _parents[head] = (UInt16)lastSym;
        _suffixes[head] = (Byte)lastChar2;
        head++;
      }
    }

    if (_parents[sym] == kNumItems)
      break;

    lastSym = sym;
    unsigned cur = sym;
    unsigned i = 0;
    
    while (cur >= 256)
    {
      _stack[i++] = _suffixes[cur];
      cur = _parents[cur];
    }
    
    _stack[i++] = (Byte)cur;
    lastChar2 = (Byte)cur;

    if (needPrev)
      _suffixes[(size_t)head - 1] = (Byte)cur;

    if (outSize)
    {
      const UInt64 limit = *outSize - nowPos;
      if (i > limit)
      {
        moreOut = true;
        i = (unsigned)limit;
      }
    }

    do
      outBuffer.WriteByte(_stack[--i]);
    while (i);
  }
  
  RINOK(outBuffer.Flush());

  if (res == S_OK)
    if (_fullStreamMode)
    {
      if (moreOut)
        res = S_FALSE;
      const UInt64 nowPos = outBuffer.GetProcessedSize();
      if (outSize && *outSize != nowPos)
        res = S_FALSE;
      if (inSize && *inSize != _inProcessed)
        res = S_FALSE;
    }
  
  return res;
}


STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  // catch(const CInBufferException &e) { return e.ErrorCode; }
  // catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(const CSystemException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}


STDMETHODIMP CDecoder::SetFinishMode(UInt32 finishMode)
{
  _fullStreamMode = (finishMode != 0);
  return S_OK;
}


STDMETHODIMP CDecoder::GetInStreamProcessedSize(UInt64 *value)
{
  *value = _inProcessed;
  return S_OK;
}


}}
