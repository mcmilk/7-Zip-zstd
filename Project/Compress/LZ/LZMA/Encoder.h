// LZMA/Encoder.h

#pragma once 

#ifndef __LZMA_ENCODER_H
#define __LZMA_ENCODER_H

#include "../../Interface/CompressInterface.h"
#include "Compression/AriPrice.h"

#include "Common/Types.h"

#include "LZMA.h"
#include "LenCoder.h"
#include "LiteralCoder.h"
#include "AriConst.h"


// {23170F69-40C1-278B-0301-010000000100}
DEFINE_GUID(CLSID_CLZMAEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00);

namespace NCompress {
namespace NLZMA {

class CMatchFinderException
{
public:
  HRESULT Result;
  CMatchFinderException(HRESULT result): Result (result) {}
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

const kIfinityPrice = 0xFFFFFFF;

typedef CMyBitEncoder<kNumMoveBitsForMainChoice> CMyBitEncoder2;

const kNumOpts = 1 << 12;

class CEncoder : 
  public ICompressCoder,
  public IInitMatchFinder,
  public ICompressSetEncoderProperties2,
  public ICompressSetCoderProperties2,
  public ICompressWriteCoderProperties,
  public CComObjectRoot,
  public CComCoClass<CEncoder, &CLSID_CLZMAEncoder>,
  public CBaseCoder
{
  COptimal _optimum[kNumOpts];
public:
  CComPtr<IInWindowStreamMatch> _matchFinder; // test it
  CMyRangeEncoder _rangeEncoder;
private:

  CMyBitEncoder2 _mainChoiceEncoders[kNumStates][NLength::kNumPosStatesEncodingMax];
  CMyBitEncoder2 _matchChoiceEncoders[kNumStates];
  CMyBitEncoder2 _matchRepChoiceEncoders[kNumStates];
  CMyBitEncoder2 _matchRep1ChoiceEncoders[kNumStates];
  CMyBitEncoder2 _matchRep2ChoiceEncoders[kNumStates];
  CMyBitEncoder2 _matchRepShortChoiceEncoders[kNumStates][NLength::kNumPosStatesEncodingMax];

  CBitTreeEncoder<kNumMoveBitsForPosSlotCoder, kNumPosSlotBits> _posSlotEncoder[kNumLenToPosStates];

  CReverseBitTreeEncoder2<kNumMoveBitsForPosCoders> _posEncoders[kNumPosModels];
  CReverseBitTreeEncoder2<kNumMoveBitsForAlignCoders> _posAlignEncoder;
  
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
    _rangeEncoder.ReleaseStream();
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

public:
  CEncoder();
  // ~CEncoder();

  HRESULT Create();

BEGIN_COM_MAP(CEncoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(IInitMatchFinder)
  COM_INTERFACE_ENTRY(ICompressSetEncoderProperties2)
  COM_INTERFACE_ENTRY(ICompressSetCoderProperties2)
  COM_INTERFACE_ENTRY(ICompressWriteCoderProperties)
  // COM_INTERFACE_ENTRY(ISetRangerEncoder)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEncoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CEncoder, TEXT("Compress.LZMAEncoder.1"), 
                 TEXT("Compress.LZMAEncoder"), UINT(0), THREADFLAGS_APARTMENT)


  STDMETHOD(Init)(ISequentialInStream *inStream, 
      ISequentialOutStream *outStream);
  
  // ICompressCoder interface
  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
  
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // IInitMatchFinder interface
  STDMETHOD(InitMatchFinder)(IInWindowStreamMatch *matchFinder);

  // ICompressSetCoderProperties2
  STDMETHOD(SetCoderProperties2)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UINT32 numProperties);
  
  // ICompressSetEncoderProperties2
  STDMETHOD(SetEncoderProperties2)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UINT32 numProperties);

  // ICompressWriteCoderProperties
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);
};

}}

#endif
