// LZMA/Encoder.h

// #pragma once 

#ifndef __LZMA_ENCODER_H
#define __LZMA_ENCODER_H

#include "../../ICoder.h"
#include "../../../Common/MyCom.h"
#include "../LZ/IMatchFinder.h"

#include "LZMA.h"
#include "LZMALen.h"
#include "LZMALiteral.h"

namespace NCompress {
namespace NLZMA {

class CMatchFinderException
{
public:
  HRESULT ErrorCode;
  CMatchFinderException(HRESULT errorCode): ErrorCode(errorCode) {}
};

struct COptimal
{
  CState State;

  bool Prev1IsChar;
  bool Prev2;

  UINT32 PosPrev2;
  UINT32 BackPrev2;     

  UINT32 Price;    
  UINT32 PosPrev;         // posNext;
  UINT32 BackPrev;     
  UINT32 Backs[kNumRepDistances];
  void MakeAsChar() { BackPrev = UINT32(-1); Prev1IsChar = false; }
  void MakeAsShortRep() { BackPrev = 0; ; Prev1IsChar = false; }
  bool IsShortRep() { return (BackPrev == 0); }
};


extern BYTE g_FastPos[1024];
inline UINT32 GetPosSlot(UINT32 pos)
{
  if (pos < (1 << 10))
    return g_FastPos[pos];
  if (pos < (1 << 19))
    return g_FastPos[pos >> 9] + 18;
  return g_FastPos[pos >> 18] + 36;
}

inline UINT32 GetPosSlot2(UINT32 pos)
{
  if (pos < (1 << 16))
    return g_FastPos[pos >> 6] + 12;
  if (pos < (1 << 25))
    return g_FastPos[pos >> 15] + 30;
  return g_FastPos[pos >> 24] + 48;
}

const UINT32 kIfinityPrice = 0xFFFFFFF;

typedef NRangeCoder::CBitEncoder<kNumMoveBits> CMyBitEncoder;

const UINT32 kNumOpts = 1 << 12;

class CEncoder : 
  public ICompressCoder,
  // public IInitMatchFinder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CBaseCoder,
  public CMyUnknownImp
{
  COptimal _optimum[kNumOpts];
public:
  CMyComPtr<IMatchFinder> _matchFinder; // test it
  NRangeCoder::CEncoder _rangeEncoder;
private:

  CMyBitEncoder _mainChoiceEncoders[kNumStates][NLength::kNumPosStatesEncodingMax];
  CMyBitEncoder _matchChoiceEncoders[kNumStates];
  CMyBitEncoder _matchRepChoiceEncoders[kNumStates];
  CMyBitEncoder _matchRep1ChoiceEncoders[kNumStates];
  CMyBitEncoder _matchRep2ChoiceEncoders[kNumStates];
  CMyBitEncoder _matchRepShortChoiceEncoders[kNumStates][NLength::kNumPosStatesEncodingMax];

  NRangeCoder::CBitTreeEncoder<kNumMoveBits, kNumPosSlotBits> _posSlotEncoder[kNumLenToPosStates];

  NRangeCoder::CReverseBitTreeEncoder2<kNumMoveBits> _posEncoders[kNumPosModels];
  NRangeCoder::CReverseBitTreeEncoder2<kNumMoveBits> _posAlignEncoder;
  
  NLength::CPriceTableEncoder _lenEncoder;
  NLength::CPriceTableEncoder _repMatchLenEncoder;

  NLiteral::CEncoder _literalEncoder;

  UINT32 _matchDistances[kMatchMaxLen + 1];

  bool _fastMode;
  bool _maxMode;
  UINT32 _numFastBytes;
  UINT32 _longestMatchLength;    

  UINT32 _additionalOffset;

  UINT32 _optimumEndIndex;
  UINT32 _optimumCurrentIndex;

  bool _longestMatchWasFound;

  UINT32 _posSlotPrices[kNumLenToPosStates][kDistTableSizeMax];
  
  UINT32 _distancesPrices[kNumLenToPosStates][kNumFullDistances];

  UINT32 _alignPrices[kAlignTableSize];
  UINT32 _alignPriceCount;

  UINT32 _distTableSize;

  UINT32 _posStateBits;
  UINT32 _posStateMask;
  UINT32 _numLiteralPosStateBits;
  UINT32 _numLiteralContextBits;

  UINT32 _dictionarySize;

  UINT32 _dictionarySizePrev;
  UINT32 _numFastBytesPrev;

  UINT64 lastPosSlotFillingPos;
  UINT64 nowPos64;
  bool _finished;
  ISequentialInStream *_inStream;

  int _matchFinderIndex;
  #ifdef COMPRESS_MF_MT
  bool _multiThread;
  #endif
  
  UINT32 ReadMatchDistances()
  {
    UINT32 len = _matchFinder->GetLongestMatch(_matchDistances);
    if (len == _numFastBytes)
      len += _matchFinder->GetMatchLen(len, _matchDistances[len], 
          kMatchMaxLen - len);
    _additionalOffset++;
    HRESULT result = _matchFinder->MovePos();
    if (result != S_OK)
      throw CMatchFinderException(result);
    return len;
  }

  void MovePos(UINT32 num);
  UINT32 GetRepLen1Price(CState state, UINT32 posState) const
  {
    return _matchRepChoiceEncoders[state.Index].GetPrice(0) +
        _matchRepShortChoiceEncoders[state.Index][posState].GetPrice(0);
  }
  UINT32 GetRepPrice(UINT32 repIndex, UINT32 len, CState state, UINT32 posState) const
  {
    UINT32 price = _repMatchLenEncoder.GetPrice(len - kMatchMinLen, posState);
    if(repIndex == 0)
    {
      price += _matchRepChoiceEncoders[state.Index].GetPrice(0);
      price += _matchRepShortChoiceEncoders[state.Index][posState].GetPrice(1);
    }
    else
    {
      price += _matchRepChoiceEncoders[state.Index].GetPrice(1);
      if (repIndex == 1)
        price += _matchRep1ChoiceEncoders[state.Index].GetPrice(0);
      else
      {
        price += _matchRep1ChoiceEncoders[state.Index].GetPrice(1);
        price += _matchRep2ChoiceEncoders[state.Index].GetPrice(repIndex - 2);
      }
    }
    return price;
  }
  /*
  UINT32 GetPosLen2Price(UINT32 pos, UINT32 posState) const
  {
    if (pos >= kNumFullDistances)
      return kIfinityPrice;
    return _distancesPrices[0][pos] + _lenEncoder.GetPrice(0, posState);
  }
  UINT32 GetPosLen3Price(UINT32 pos, UINT32 len, UINT32 posState) const
  {
    UINT32 price;
    UINT32 lenToPosState = GetLenToPosState(len);
    if (pos < kNumFullDistances)
      price = _distancesPrices[lenToPosState][pos];
    else
      price = _posSlotPrices[lenToPosState][GetPosSlot2(pos)] + 
          _alignPrices[pos & kAlignMask];
    return price + _lenEncoder.GetPrice(len - kMatchMinLen, posState);
  }
  */
  UINT32 GetPosLenPrice(UINT32 pos, UINT32 len, UINT32 posState) const
  {
    if (len == 2 && pos >= 0x80)
      return kIfinityPrice;
    UINT32 price;
    UINT32 lenToPosState = GetLenToPosState(len);
    if (pos < kNumFullDistances)
      price = _distancesPrices[lenToPosState][pos];
    else
      price = _posSlotPrices[lenToPosState][GetPosSlot2(pos)] + 
          _alignPrices[pos & kAlignMask];
    return price + _lenEncoder.GetPrice(len - kMatchMinLen, posState);
  }

  UINT32 Backward(UINT32 &backRes, UINT32 cur);
  UINT32 GetOptimum(UINT32 &backRes, UINT32 position);
  UINT32 GetOptimumFast(UINT32 &backRes, UINT32 position);

  void FillPosSlotPrices();
  void FillDistancesPrices();
  void FillAlignPrices();
    
  void ReleaseStreams()
  {
    _matchFinder->ReleaseStream();
    // _rangeEncoder.ReleaseStream();
  }

  HRESULT Flush();
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

  void WriteEndMarker(UINT32 posState);

public:
  CEncoder();
  // ~CEncoder();

  HRESULT Create();

  MY_UNKNOWN_IMP2(
      ICompressSetCoderProperties,
      ICompressWriteCoderProperties
      )
    
  STDMETHOD(Init)(
      ISequentialOutStream *outStream);
  
  // ICompressCoder interface
  HRESULT SetStreams(ISequentialInStream *inStream,
      ISequentialOutStream *outStream,
      const UINT64 *inSize, const UINT64 *outSize);
  HRESULT CodeOneBlock(UINT64 *inSize, UINT64 *outSize, INT32 *finished);

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // ICompressCoder interface
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // IInitMatchFinder interface
  STDMETHOD(InitMatchFinder)(IMatchFinder *matchFinder);

  // ICompressSetCoderProperties2
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UINT32 numProperties);
  
  // ICompressWriteCoderProperties
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);
};

}}

#endif
