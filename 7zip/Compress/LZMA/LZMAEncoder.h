// LZMA/Encoder.h

#ifndef __LZMA_ENCODER_H
#define __LZMA_ENCODER_H

#include "../../../Common/MyCom.h"
#include "../../../Common/Alloc.h"
#include "../../ICoder.h"
#include "../LZ/IMatchFinder.h"
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


extern Byte g_FastPos[1024];
inline UInt32 GetPosSlot(UInt32 pos)
{
  if (pos < (1 << 10))
    return g_FastPos[pos];
  if (pos < (1 << 19))
    return g_FastPos[pos >> 9] + 18;
  return g_FastPos[pos >> 18] + 36;
}

inline UInt32 GetPosSlot2(UInt32 pos)
{
  if (pos < (1 << 16))
    return g_FastPos[pos >> 6] + 12;
  if (pos < (1 << 25))
    return g_FastPos[pos >> 15] + 30;
  return g_FastPos[pos >> 24] + 48;
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
    if (_coders == 0 || (numPosBits + numPrevBits) != 
        (_numPrevBits + _numPosBits) )
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
  UInt32 GetState(UInt32 pos, Byte prevByte) const
    { return ((pos & _posMask) << _numPrevBits) + (prevByte >> (8 - _numPrevBits)); }
  CLiteralEncoder2 *GetSubCoder(UInt32 pos, Byte prevByte)
    { return &_coders[GetState(pos, prevByte)]; }
  /*
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UInt32 pos, Byte prevByte, 
      Byte symbol)
    { _coders[GetState(pos, prevByte)].Encode(rangeEncoder, symbol); }
  void EncodeMatched(NRangeCoder::CEncoder *rangeEncoder, UInt32 pos, Byte prevByte, 
      Byte matchByte, Byte symbol)
    { _coders[GetState(pos, prevByte)].Encode(rangeEncoder,
      matchByte, symbol); }
  */
  UInt32 GetPrice(UInt32 pos, Byte prevByte, bool matchMode, Byte matchByte, Byte symbol) const
    { return _coders[GetState(pos, prevByte)].GetPrice(matchMode, matchByte, symbol); }
};

namespace NLength {

class CEncoder
{
  CMyBitEncoder _choice;
  CMyBitEncoder  _choice2;
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumLowBits>  _lowCoder[kNumPosStatesEncodingMax];
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumMidBits>  _midCoder[kNumPosStatesEncodingMax];
  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumHighBits>  _highCoder;
public:
  void Init(UInt32 numPosStates);
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UInt32 symbol, UInt32 posState);
  UInt32 GetPrice(UInt32 symbol, UInt32 posState) const;
};

const UInt32 kNumSpecSymbols = kNumLowSymbols + kNumMidSymbols;

class CPriceTableEncoder: public CEncoder
{
  UInt32 _prices[kNumSymbolsTotal][kNumPosStatesEncodingMax];
  UInt32 _tableSize;
  UInt32 _counters[kNumPosStatesEncodingMax];
public:
  void SetTableSize(UInt32 tableSize) { _tableSize = tableSize;  }
  UInt32 GetPrice(UInt32 symbol, UInt32 posState) const
    { return _prices[symbol][posState]; }
  void UpdateTable(UInt32 posState)
  {
    for (UInt32 len = 0; len < _tableSize; len++)
      _prices[len][posState] = CEncoder::GetPrice(len, posState);
    _counters[posState] = _tableSize;
  }
  void UpdateTables(UInt32 numPosStates)
  {
    for (UInt32 posState = 0; posState < numPosStates; posState++)
      UpdateTable(posState);
  }
  void Encode(NRangeCoder::CEncoder *rangeEncoder, UInt32 symbol, UInt32 posState)
  {
    CEncoder::Encode(rangeEncoder, symbol, posState);
    if (--_counters[posState] == 0)
      UpdateTable(posState);
  }
};

}

class CEncoder : 
  public ICompressCoder,
  public ICompressSetOutStream,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CBaseState,
  public CMyUnknownImp
{
  COptimal _optimum[kNumOpts];
  CMyComPtr<IMatchFinder> _matchFinder; // test it
  NRangeCoder::CEncoder _rangeEncoder;

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

  UInt32 _matchDistances[kMatchMaxLen + 1];

  bool _fastMode;
  bool _maxMode;
  UInt32 _numFastBytes;
  UInt32 _longestMatchLength;    

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

  UInt32 _dictionarySizePrev;
  UInt32 _numFastBytesPrev;

  UInt64 lastPosSlotFillingPos;
  UInt64 nowPos64;
  bool _finished;
  ISequentialInStream *_inStream;

  int _matchFinderIndex;
  #ifdef COMPRESS_MF_MT
  bool _multiThread;
  #endif

  bool _writeEndMark;

  bool _needReleaseMFStream;
  
  HRESULT ReadMatchDistances(UInt32 &len);

  HRESULT MovePos(UInt32 num);
  UInt32 GetRepLen1Price(CState state, UInt32 posState) const
  {
    return _isRepG0[state.Index].GetPrice0() +
        _isRep0Long[state.Index][posState].GetPrice0();
  }
  UInt32 GetRepPrice(UInt32 repIndex, UInt32 len, CState state, UInt32 posState) const
  {
    UInt32 price = _repMatchLenEncoder.GetPrice(len - kMatchMinLen, posState);
    if(repIndex == 0)
    {
      price += _isRepG0[state.Index].GetPrice0();
      price += _isRep0Long[state.Index][posState].GetPrice1();
    }
    else
    {
      price += _isRepG0[state.Index].GetPrice1();
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
    if (len == 2 && pos >= 0x80)
      return kIfinityPrice;
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
  HRESULT GetOptimum(UInt32 position, UInt32 &backRes, UInt32 &lenRes);
  HRESULT GetOptimumFast(UInt32 position, UInt32 &backRes, UInt32 &lenRes);

  void FillPosSlotPrices();
  void FillDistancesPrices();
  void FillAlignPrices();
    
  void ReleaseMFStream()
  {
    if (_matchFinder && _needReleaseMFStream)
    {
      _matchFinder->ReleaseStream();
      _needReleaseMFStream = false;
    }
  }

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
    ~CCoderReleaser()
    {
      _coder->ReleaseStreams();
    }
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

  // IInitMatchFinder interface
  STDMETHOD(InitMatchFinder)(IMatchFinder *matchFinder);

  // ICompressSetCoderProperties2
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties);
  
  // ICompressWriteCoderProperties
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  STDMETHOD(SetOutStream)(ISequentialOutStream *outStream);
  STDMETHOD(ReleaseOutStream)();

  virtual ~CEncoder() {}
};

}}

#endif
