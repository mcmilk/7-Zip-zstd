// QuantumDecoder.cpp

#include "StdAfx.h"

#include "../../Common/Defs.h"

#include "QuantumDecoder.h"

namespace NCompress {
namespace NQuantum {

static const int kLenIdNeedInit = -2;

static const unsigned kNumLenSymbols = 27;
static const unsigned kMatchMinLen = 3;
static const unsigned kNumSimplePosSlots = 4;
static const unsigned kNumSimpleLenSlots = 6;

void CDecoder::Init()
{
  m_Selector.Init(kNumSelectors);
  unsigned i;
  for (i = 0; i < kNumLitSelectors; i++)
    m_Literals[i].Init(kNumLitSymbols);
  unsigned numItems = (_numDictBits == 0 ? 1 : (_numDictBits << 1));
  const unsigned kNumPosSymbolsMax[kNumMatchSelectors] = { 24, 36, 42 };
  for (i = 0; i < kNumMatchSelectors; i++)
    m_PosSlot[i].Init(MyMin(numItems, kNumPosSymbolsMax[i]));
  m_LenSlot.Init(kNumLenSymbols);
}

HRESULT CDecoder::CodeSpec(UInt32 curSize)
{
  if (_remainLen == kLenIdNeedInit)
  {
    if (!_keepHistory)
    {
      if (!_outWindowStream.Create((UInt32)1 << _numDictBits))
        return E_OUTOFMEMORY;
      Init();
    }
    if (!_rangeDecoder.Create(1 << 20))
      return E_OUTOFMEMORY;
    _rangeDecoder.Init();
    _remainLen = 0;
  }
  if (curSize == 0)
    return S_OK;

  while (_remainLen > 0 && curSize > 0)
  {
    _remainLen--;
    Byte b = _outWindowStream.GetByte(_rep0);
    _outWindowStream.PutByte(b);
    curSize--;
  }

  while (curSize > 0)
  {
    if (_rangeDecoder.Stream.WasFinished())
      return S_FALSE;

    unsigned selector = m_Selector.Decode(&_rangeDecoder);
    if (selector < kNumLitSelectors)
    {
      Byte b = (Byte)((selector << (8 - kNumLitSelectorBits)) + m_Literals[selector].Decode(&_rangeDecoder));
      _outWindowStream.PutByte(b);
      curSize--;
    }
    else
    {
      selector -= kNumLitSelectors;
      unsigned len = selector + kMatchMinLen;
      if (selector == 2)
      {
        unsigned lenSlot = m_LenSlot.Decode(&_rangeDecoder);
        if (lenSlot >= kNumSimpleLenSlots)
        {
          lenSlot -= 2;
          int numDirectBits = (int)(lenSlot >> 2);
          len +=  ((4 | (lenSlot & 3)) << numDirectBits) - 2;
          if (numDirectBits < 6)
            len += _rangeDecoder.Stream.ReadBits(numDirectBits);
        }
        else
          len += lenSlot;
      }
      UInt32 rep0 = m_PosSlot[selector].Decode(&_rangeDecoder);
      if (rep0 >= kNumSimplePosSlots)
      {
        int numDirectBits = (int)((rep0 >> 1) - 1);
        rep0 = ((2 | (rep0 & 1)) << numDirectBits) + _rangeDecoder.Stream.ReadBits(numDirectBits);
      }
      unsigned locLen = len;
      if (len > curSize)
        locLen = (unsigned)curSize;
      if (!_outWindowStream.CopyBlock(rep0, locLen))
        return S_FALSE;
      curSize -= locLen;
      len -= locLen;
      if (len != 0)
      {
        _remainLen = (int)len;
        _rep0 = rep0;
        break;
      }
    }
  }
  return _rangeDecoder.Stream.WasFinished() ? S_FALSE : S_OK;
}

HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  if (outSize == NULL)
    return E_INVALIDARG;
  UInt64 size = *outSize;

  SetInStream(inStream);
  _outWindowStream.SetStream(outStream);
  SetOutStreamSize(outSize);
  CDecoderFlusher flusher(this);

  const UInt64 start = _outWindowStream.GetProcessedSize();
  for (;;)
  {
    UInt32 curSize = 1 << 18;
    UInt64 rem = size - (_outWindowStream.GetProcessedSize() - start);
    if (curSize > rem)
      curSize = (UInt32)rem;
    if (curSize == 0)
      break;
    RINOK(CodeSpec(curSize));
    if (progress != NULL)
    {
      UInt64 inSize = _rangeDecoder.GetProcessedSize();
      UInt64 nowPos64 = _outWindowStream.GetProcessedSize() - start;
      RINOK(progress->SetRatioInfo(&inSize, &nowPos64));
    }
  }
  flusher.NeedFlush = false;
  return Flush();
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  try  { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e)  { return e.ErrorCode; }
  catch(const CLzOutWindowException &e)  { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
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
  if (outSize == NULL)
    return E_FAIL;
  _remainLen = kLenIdNeedInit;
  _outWindowStream.Init(_keepHistory);
  return S_OK;
}

}}
