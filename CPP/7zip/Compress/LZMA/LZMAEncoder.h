// LZMA/Encoder.h

#ifndef __LZMA_ENCODER_H
#define __LZMA_ENCODER_H

#include "../../../Common/MyCom.h"
#include "../../ICoder.h"

extern "C"
{
  #include "../../../../C/Alloc.h"
  #include "../../../../C/Compress/Lz/MatchFinder.h"
  #ifdef COMPRESS_MF_MT
  #include "../../../../C/Compress/Lz/MatchFinderMt.h"
  #endif
}

#include "../RangeCoder/RangeCoderBitTree.h"

#include "LZMA.h"

namespace NCompress {
namespace NLZMA {

typedef NRangeCoder::CBitEncoder<kNumMoveBits> CMyBitEncoder;

class CBaseState
{
protected:
  CState _state;
  Byte _previousByte;
  UInt32 _repDistances[kNumRepDistances];
  void Init()
  {
    _state.Init();
    _previousByte = 0;
    for(UInt32 i = 0 ; i < kNumRepDistances; i++)
      _repDistances[i] = 0;
  }
};

struct COptimal
{
  CState State;

  bool Prev1IsChar;
  bool Prev2;

  UInt32 PosPrev2;
  UInt32 BackPrev2;     

  UInt32 Price;    
  UInt32 PosPrev;         // posNext;
  UInt32 BackPrev;     
  UInt32 Backs[kNumRepDistances];
  void MakeAsChar() { BackPrev = UInt32(-1); Prev1IsChar = false; }
  void MakeAsShortRep() { BackPrev = 0; ; Prev1IsChar = false; }
  bool IsShortRep() { return (BackPrev == 0); }
};


// #define LZMA_LOG_BRANCH

#if _MSC_VER >= 1400
// Must give gain in core 2. but slower ~2% on k8.
// #define LZMA_LOG_BSR
#endif

#ifndef LZMA_LOG_BSR
static const int kNumLogBits = 13; // don't change it !
extern Byte g_FastPos[];
#endif
inline UInt32 GetPosSlot(UInt32 pos)
{
  #ifdef LZMA_LOG_BSR
  if (pos < 2)
    return pos;
  unsigned long index;
  _BitScanReverse(&index, pos);
  return (index + index) + ((pos >> (index - 1)) & 1);
  #else
  if (pos < (1 << kNumLogBits))
    return g_FastPos[pos];
  if (pos < (1 << (kNumLogBits * 2 - 1)))
    return g_FastPos[pos >> (kNumLogBits - 1)] + (kNumLogBits - 1) * 2;
  return g_FastPos[pos >> (kNumLogBits - 1) * 2] + (kNumLogBits - 1) * 4;
  #endif
}

inline UInt32 GetPosSlot2(UInt32 pos)
{
  #ifdef LZMA_LOG_BSR
  unsigned long index;
  _BitScanReverse(&index, pos);
  return (index + index) + ((pos >> (index - 1)) & 1);
  #else
  #ifdef LZMA_LOG_BRANCH
  if (pos < (1 << (kNumLogBits + 6)))
    return g_FastPos[pos >> 6] + 12;
  if (pos < (1 << (kNumLogBits * 2 + 5)))
    return g_FastPos[pos >> (kNumLogBits + 5)] + (kNumLogBits + 5) * 2;
  return g_FastPos[pos >> (kNumLogBits * 2 + 4)] + (kNumLogBits * 2 + 4) * 2;
  #else
  // it's faster with VC6-32bit.
  UInt32 s = 6 + ((kNumLogBits - 1) & (UInt32)((Int32)(((1 << (kNumLogBits + 6)) - 1) -  pos) >> 31));
  return g_FastPos[pos >> s] + (s * 2);
  #endif
  #endif
}

const UInt32 kIfinityPrice = 0xFFFFFFF;

const UInt32 kNumOpts = 1 << 12;


class CLiteralEncoder2
{
  CMyBitEncoder _encoders[0x300];
public:
  void Init()
  {
    for (int i = 0; i < 0x300; i++)
      _encoders[i].Init();
  }
  void Encode(NRangeCoder::CEncoder *rangeEncoder, Byte symbol);
  void EncodeMatched(NRangeCoder::CEncoder *rangeEncoder, Byte matchByte, Byte symbol);
  UInt32 GetPrice(bool matchMode, Byte matchByte, Byte symbol) const;
};

class CLiteralEncoder
{
  CLiteralEncoder2 *_coders;
  int _numPrevBits;
  int _numPosBits;
  UInt32 _posMask;
public:
  CLiteralEncoder(): _coders(0) {}
  ~CLiteralEncoder()  { Free(); }
  void Free()
  { 
    MyFree(_coders);
    _coders = 0;
  }
  bool Create(int numPosBits, int numPrevBits)
  {
    if (_coders == 0 || (numPosBits + numPrevBits) != (_numPrevBits + _numPosBits))
    {
      Free();
      UInt32 numStates = 1 << (numPosBits + numPrevBits);
      _coders = (CLiteralEncoder2 *)MyAlloc(numStates * sizeof(CLiteralEncoder2));
    }
    _numPosBits = numPosBits;
    _posMask = (1 << numPosBits) - 1;
    _numPrevBits = numPrevBits;
    return (_coders != 0);
  }
  void Init()
  {
    UInt32 numStates = 1 << (_numPrevBits + _numPosBits);
    for (UInt32 i = 0; i < numStates; i++)
      _coders[i].Init();
  }
  CLiteralEncoder2 *GetSubCoder(UInt32 pos, Byte prevByte)
    { return &_coders[((pos & _posMask) << _numPrevBits) + (prevByte >> (8 - _numPrevBits))]; }
};

namespace NLength {

class CEncoder
{
  CMyBitEncoder _choice;
  CMyBitEncoder _choice2;
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumLowBits> _lowCoder[kNumPosStatesEncodingMax];
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumMidBits> _midCoder[kNumPosStatesEncodingMax];
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumHighBits> _highCoder;
public:
  void Init(UInt32 numPosStates);
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UInt32 symbol, UInt32 posState);
  void SetPrices(UInt32 posState, UInt32 numSymbols, UInt32 *prices) const;
};

const UInt32 kNumSpecSymbols = kNumLowSymbols + kNumMidSymbols;

class CPriceTableEncoder: public CEncoder
{
  UInt32 _prices[kNumPosStatesEncodingMax][kNumSymbolsTotal];
  UInt32 _tableSize;
  UInt32 _counters[kNumPosStatesEncodingMax];
public:
  void SetTableSize(UInt32 tableSize) { _tableSize = tableSize;  }
  UInt32 GetPrice(UInt32 symbol, UInt32 posState) const { return _prices[posState][symbol]; }
  void UpdateTable(UInt32 posState)
  {
    SetPrices(posState, _tableSize, _prices[posState]);
    _counters[posState] = _tableSize;
  }
  void UpdateTables(UInt32 numPosStates)
  {
    for (UInt32 posState = 0; posState < numPosStates; posState++)
      UpdateTable(posState);
  }
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UInt32 symbol, UInt32 posState, bool updatePrice)
  {
    CEncoder::Encode(rangeEncoder, symbol, posState);
    if (updatePrice)
      if (--_counters[posState] == 0)
        UpdateTable(posState);
  }
};

}

typedef struct _CSeqInStream
{
  ISeqInStream SeqInStream;
  CMyComPtr<ISequentialInStream> RealStream;
} CSeqInStream;


class CEncoder : 
  public ICompressCoder,
  public ICompressSetOutStream,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CBaseState,
  public CMyUnknownImp
{
  NRangeCoder::CEncoder _rangeEncoder;

  IMatchFinder _matchFinder;
  void *_matchFinderObj;
  
  #ifdef COMPRESS_MF_MT
  Bool _multiThread;
  Bool _mtMode;
  CMatchFinderMt _matchFinderMt;
  #endif

  CMatchFinder _matchFinderBase;
  #ifdef COMPRESS_MF_MT
  Byte _pad1[kMtCacheLineDummy];
  #endif

  COptimal _optimum[kNumOpts];

  CMyBitEncoder _isMatch[kNumStates][NLength::kNumPosStatesEncodingMax];
  CMyBitEncoder _isRep[kNumStates];
  CMyBitEncoder _isRepG0[kNumStates];
  CMyBitEncoder _isRepG1[kNumStates];
  CMyBitEncoder _isRepG2[kNumStates];
  CMyBitEncoder _isRep0Long[kNumStates][NLength::kNumPosStatesEncodingMax];

  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumPosSlotBits> _posSlotEncoder[kNumLenToPosStates];

  CMyBitEncoder _posEncoders[kNumFullDistances - kEndPosModelIndex];
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumAlignBits> _posAlignEncoder;
  
  NLength::CPriceTableEncoder _lenEncoder;
  NLength::CPriceTableEncoder _repMatchLenEncoder;

  CLiteralEncoder _literalEncoder;

  UInt32 _matchDistances[kMatchMaxLen * 2 + 2 + 1];

  bool _fastMode;
  // bool _maxMode;
  UInt32 _numFastBytes;
  UInt32 _longestMatchLength;    
  UInt32 _numDistancePairs;

  UInt32 _additionalOffset;

  UInt32 _optimumEndIndex;
  UInt32 _optimumCurrentIndex;

  bool _longestMatchWasFound;

  UInt32 _posSlotPrices[kNumLenToPosStates][kDistTableSizeMax];
  
  UInt32 _distancesPrices[kNumLenToPosStates][kNumFullDistances];

  UInt32 _alignPrices[kAlignTableSize];
  UInt32 _alignPriceCount;

  UInt32 _distTableSize;

  UInt32 _posStateBits;
  UInt32 _posStateMask;
  UInt32 _numLiteralPosStateBits;
  UInt32 _numLiteralContextBits;

  UInt32 _dictionarySize;

  UInt32 _matchPriceCount;
  UInt64 nowPos64;
  bool _finished;
  ISequentialInStream *_inStream;

  CSeqInStream _seqInStream;

  UInt32 _matchFinderCycles;
  // int _numSkip

  bool _writeEndMark;

  bool _needReleaseMFStream;

  void ReleaseMatchFinder()
  {
    _matchFinder.Init = 0;
    _seqInStream.RealStream.Release();
  }

  void ReleaseMFStream()
  {
    if (_matchFinderObj && _needReleaseMFStream)
    {
      #ifdef COMPRESS_MF_MT
      if (_mtMode)
        MatchFinderMt_ReleaseStream(&_matchFinderMt);
      #endif
      _needReleaseMFStream = false;
    }
    _seqInStream.RealStream.Release();
  }
  
  UInt32 ReadMatchDistances(UInt32 &numDistancePairs);

  void MovePos(UInt32 num);
  UInt32 GetRepLen1Price(CState state, UInt32 posState) const
  {
    return _isRepG0[state.Index].GetPrice0() +
        _isRep0Long[state.Index][posState].GetPrice0();
  }
  
  UInt32 GetPureRepPrice(UInt32 repIndex, CState state, UInt32 posState) const
  {
    UInt32 price;
    if(repIndex == 0)
    {
      price = _isRepG0[state.Index].GetPrice0();
      price += _isRep0Long[state.Index][posState].GetPrice1();
    }
    else
    {
      price = _isRepG0[state.Index].GetPrice1();
      if (repIndex == 1)
        price += _isRepG1[state.Index].GetPrice0();
      else
      {
        price += _isRepG1[state.Index].GetPrice1();
        price += _isRepG2[state.Index].GetPrice(repIndex - 2);
      }
    }
    return price;
  }
  UInt32 GetRepPrice(UInt32 repIndex, UInt32 len, CState state, UInt32 posState) const
  {
    return _repMatchLenEncoder.GetPrice(len - kMatchMinLen, posState) +
        GetPureRepPrice(repIndex, state, posState);
  }
  /*
  UInt32 GetPosLen2Price(UInt32 pos, UInt32 posState) const
  {
    if (pos >= kNumFullDistances)
      return kIfinityPrice;
    return _distancesPrices[0][pos] + _lenEncoder.GetPrice(0, posState);
  }
  UInt32 GetPosLen3Price(UInt32 pos, UInt32 len, UInt32 posState) const
  {
    UInt32 price;
    UInt32 lenToPosState = GetLenToPosState(len);
    if (pos < kNumFullDistances)
      price = _distancesPrices[lenToPosState][pos];
    else
      price = _posSlotPrices[lenToPosState][GetPosSlot2(pos)] + 
          _alignPrices[pos & kAlignMask];
    return price + _lenEncoder.GetPrice(len - kMatchMinLen, posState);
  }
  */
  UInt32 GetPosLenPrice(UInt32 pos, UInt32 len, UInt32 posState) const
  {
    UInt32 price;
    UInt32 lenToPosState = GetLenToPosState(len);
    if (pos < kNumFullDistances)
      price = _distancesPrices[lenToPosState][pos];
    else
      price = _posSlotPrices[lenToPosState][GetPosSlot2(pos)] + 
          _alignPrices[pos & kAlignMask];
    return price + _lenEncoder.GetPrice(len - kMatchMinLen, posState);
  }

  UInt32 Backward(UInt32 &backRes, UInt32 cur);
  UInt32 GetOptimum(UInt32 position, UInt32 &backRes);
  UInt32 GetOptimumFast(UInt32 &backRes);

  void FillDistancesPrices();
  void FillAlignPrices();
    
  void ReleaseStreams()
  {
    ReleaseMFStream();
    ReleaseOutStream();
  }

  HRESULT Flush(UInt32 nowPos);
  class CCoderReleaser
  {
    CEncoder *_coder;
  public:
    CCoderReleaser(CEncoder *coder): _coder(coder) {}
    ~CCoderReleaser() { _coder->ReleaseStreams(); }
  };
  friend class CCoderReleaser;

  void WriteEndMarker(UInt32 posState);

public:
  CEncoder();
  void SetWriteEndMarkerMode(bool writeEndMarker)
    { _writeEndMark= writeEndMarker; }

  HRESULT Create();

  MY_UNKNOWN_IMP3(
      ICompressSetOutStream,
      ICompressSetCoderProperties,
      ICompressWriteCoderProperties
      )
    
  HRESULT Init();
  
  // ICompressCoder interface
  HRESULT SetStreams(ISequentialInStream *inStream,
      ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize);
  HRESULT CodeOneBlock(UInt64 *inSize, UInt64 *outSize, Int32 *finished);

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  // ICompressCoder interface
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  // ICompressSetCoderProperties2
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties);
  
  // ICompressWriteCoderProperties
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  STDMETHOD(SetOutStream)(ISequentialOutStream *outStream);
  STDMETHOD(ReleaseOutStream)();

  virtual ~CEncoder();
};

}}

#endif
