// QuantumDecoder.h

#ifndef __COMPRESS_QUANTUM_DECODER_H
#define __COMPRESS_QUANTUM_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "LzOutWindow.h"

namespace NCompress {
namespace NQuantum {

class CStreamBitDecoder
{
  UInt32 Value;
  CInBuffer Stream;
public:
  bool Create(UInt32 bufferSize) { return Stream.Create(bufferSize); }
  void SetStream(ISequentialInStream *stream) { Stream.SetStream(stream); }
  void ReleaseStream() { Stream.ReleaseStream(); }

  void Finish() { Value = 0x10000; }

  void Init()
  {
    Stream.Init();
    Value = 0x10000;
  }

  UInt64 GetProcessedSize() const { return Stream.GetProcessedSize(); }
  bool WasFinished() const { return Stream.WasFinished(); }
  
  UInt32 ReadBit()
  {
    if (Value >= 0x10000)
      Value = 0x100 | Stream.ReadByte();
    UInt32 res = (Value >> 7) & 1;
    Value <<= 1;
    return res;
  }

  UInt32 ReadBits(int numBits) // numBits > 0
  {
    UInt32 res = 0;
    do
      res = (res << 1) | ReadBit();
    while (--numBits != 0);
    return res;
  }
};

const unsigned kNumLitSelectorBits = 2;
const unsigned kNumLitSelectors = (1 << kNumLitSelectorBits);
const unsigned kNumLitSymbols = 1 << (8 - kNumLitSelectorBits);
const unsigned kNumMatchSelectors = 3;
const unsigned kNumSelectors = kNumLitSelectors + kNumMatchSelectors;
const unsigned kNumSymbolsMax = kNumLitSymbols; // 64

namespace NRangeCoder {

class CDecoder
{
  UInt32 Low;
  UInt32 Range;
  UInt32 Code;
public:
  CStreamBitDecoder Stream;
  bool Create(UInt32 bufferSize) { return Stream.Create(bufferSize); }
  void SetStream(ISequentialInStream *stream) { Stream.SetStream(stream); }
  void ReleaseStream() { Stream.ReleaseStream(); }

  void Init()
  {
    Stream.Init();
    Low = 0;
    Range = 0x10000;
    Code = Stream.ReadBits(16);
  }

  void Finish()
  {
    // we need these extra two Bit_reads
    Stream.ReadBit();
    Stream.ReadBit();
    Stream.Finish();
  }

  UInt64 GetProcessedSize() const { return Stream.GetProcessedSize(); }

  UInt32 GetThreshold(UInt32 total) const
  {
    return ((Code + 1) * total - 1) / Range; // & 0xFFFF is not required;
  }

  void Decode(UInt32 start, UInt32 end, UInt32 total)
  {
    UInt32 high = Low + end * Range / total - 1;
    UInt32 offset = start * Range / total;
    Code -= offset;
    Low += offset;
    for (;;)
    {
      if ((Low & 0x8000) != (high & 0x8000))
      {
        if ((Low & 0x4000) == 0 || (high & 0x4000) != 0)
          break;
        Low &= 0x3FFF;
        high |= 0x4000;
      }
      Low = (Low << 1) & 0xFFFF;
      high = ((high << 1) | 1) & 0xFFFF;
      Code = ((Code << 1) | Stream.ReadBit());
    }
    Range = high - Low + 1;
  }
};

const UInt16 kUpdateStep = 8;
const UInt16 kFreqSumMax = 3800;
const UInt16 kReorderCountStart = 4;
const UInt16 kReorderCount = 50;

class CModelDecoder
{
  unsigned NumItems;
  unsigned ReorderCount;
  UInt16 Freqs[kNumSymbolsMax + 1];
  Byte Values[kNumSymbolsMax];
public:
  void Init(unsigned numItems)
  {
    NumItems = numItems;
    ReorderCount = kReorderCountStart;
    for (unsigned i = 0; i < numItems; i++)
    {
      Freqs[i] = (UInt16)(numItems - i);
      Values[i] = (Byte)i;
    }
    Freqs[numItems] = 0;
  }
  
  unsigned Decode(CDecoder *rangeDecoder)
  {
    UInt32 threshold = rangeDecoder->GetThreshold(Freqs[0]);
    unsigned i;
    for (i = 1; Freqs[i] > threshold; i++);
    rangeDecoder->Decode(Freqs[i], Freqs[i - 1], Freqs[0]);
    unsigned res = Values[--i];
    do
      Freqs[i] += kUpdateStep;
    while (i-- != 0);

    if (Freqs[0] > kFreqSumMax)
    {
      if (--ReorderCount == 0)
      {
        ReorderCount = kReorderCount;
        for (i = 0; i < NumItems; i++)
          Freqs[i] = (UInt16)(((Freqs[i] - Freqs[i + 1]) + 1) >> 1);
        for (i = 0; i < NumItems - 1; i++)
          for (unsigned j = i + 1; j < NumItems; j++)
            if (Freqs[i] < Freqs[j])
            {
              UInt16 tmpFreq = Freqs[i];
              Byte tmpVal = Values[i];
              Freqs[i] = Freqs[j];
              Values[i] = Values[j];
              Freqs[j] = tmpFreq;
              Values[j] = tmpVal;
            }
        do
          Freqs[i] = (UInt16)(Freqs[i] + Freqs[i + 1]);
        while (i-- != 0);
      }
      else
      {
        i = NumItems - 1;
        do
        {
          Freqs[i] >>= 1;
          if (Freqs[i] <= Freqs[i + 1])
            Freqs[i] = (UInt16)(Freqs[i + 1] + 1);
        }
        while (i-- != 0);
      }
    }
    return res;
  }
};

}

class CDecoder:
  public ICompressCoder,
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public CMyUnknownImp
{
  CLzOutWindow _outWindowStream;
  NRangeCoder::CDecoder _rangeDecoder;

  UInt64 _outSize;
  int _remainLen; // -1 means end of stream. // -2 means need Init
  UInt32 _rep0;

  int _numDictBits;
  bool _keepHistory;

  NRangeCoder::CModelDecoder m_Selector;
  NRangeCoder::CModelDecoder m_Literals[kNumLitSelectors];
  NRangeCoder::CModelDecoder m_PosSlot[kNumMatchSelectors];
  NRangeCoder::CModelDecoder m_LenSlot;
  void Init();
  HRESULT CodeSpec(UInt32 size);
public:
  MY_UNKNOWN_IMP2(
      ICompressSetInStream,
      ICompressSetOutStreamSize)

  void ReleaseStreams()
  {
    _outWindowStream.ReleaseStream();
    ReleaseInStream();
  }

  class CDecoderFlusher
  {
    CDecoder *_decoder;
  public:
    bool NeedFlush;
    CDecoderFlusher(CDecoder *decoder): _decoder(decoder), NeedFlush(true) {}
    ~CDecoderFlusher()
    {
      if (NeedFlush)
        _decoder->Flush();
      _decoder->ReleaseStreams();
    }
  };

  HRESULT Flush() {  return _outWindowStream.Flush(); }

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  STDMETHOD(SetInStream)(ISequentialInStream *inStream);
  STDMETHOD(ReleaseInStream)();
  STDMETHOD(SetOutStreamSize)(const UInt64 *outSize);

  void SetParams(int numDictBits) { _numDictBits = numDictBits; }
  void SetKeepHistory(bool keepHistory) { _keepHistory = keepHistory; }
  CDecoder(): _keepHistory(false) {}
  virtual ~CDecoder() {}
};

}}

#endif
