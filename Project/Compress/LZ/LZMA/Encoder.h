// LZArithmetic/Encoder.h

#pragma once 

#ifndef __LZARITHMETIC_ENCODER_H
#define __LZARITHMETIC_ENCODER_H

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
  HRESULT m_Result;
  CMatchFinderException(HRESULT aResult): m_Result (aResult) {}
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
inline UINT32 GetPosSlot(UINT32 aPos)
{
  if (aPos < (1 << 10))
    return g_FastPos[aPos];
  if (aPos < (1 << 19))
    return g_FastPos[aPos >> 9] + 18;
  return g_FastPos[aPos >> 18] + 36;
}

inline UINT32 GetPosSlot2(UINT32 aPos)
{
  if (aPos < (1 << 16))
    return g_FastPos[aPos >> 6] + 12;
  if (aPos < (1 << 25))
    return g_FastPos[aPos >> 15] + 30;
  return g_FastPos[aPos >> 24] + 48;
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
  COptimal m_Optimum[kNumOpts];
public:
  CComPtr<IInWindowStreamMatch> m_MatchFinder; // test it
  CMyRangeEncoder m_RangeEncoder;
private:

  CMyBitEncoder2 m_MainChoiceEncoders[kNumStates][NLength::kNumPosStatesEncodingMax];
  CMyBitEncoder2 m_MatchChoiceEncoders[kNumStates];
  CMyBitEncoder2 m_MatchRepChoiceEncoders[kNumStates];
  CMyBitEncoder2 m_MatchRep1ChoiceEncoders[kNumStates];
  CMyBitEncoder2 m_MatchRep2ChoiceEncoders[kNumStates];
  CMyBitEncoder2 m_MatchRepShortChoiceEncoders[kNumStates][NLength::kNumPosStatesEncodingMax];

  CBitTreeEncoder<kNumMoveBitsForPosSlotCoder, kNumPosSlotBits> m_PosSlotEncoder[kNumLenToPosStates];

  CReverseBitTreeEncoder2<kNumMoveBitsForPosCoders> m_PosEncoders[kNumPosModels];
  CReverseBitTreeEncoder2<kNumMoveBitsForAlignCoders> m_PosAlignEncoder;
  // CBitTreeEncoder2<kNumMoveBitsForPosCoders> m_PosEncoders[kNumPosModels];
  // CBitTreeEncoder2<kNumMoveBitsForAlignCoders> m_PosAlignEncoder;
  
  NLength::CPriceTableEncoder m_LenEncoder;
  NLength::CPriceTableEncoder m_RepMatchLenEncoder;

  NLiteral::CEncoder m_LiteralEncoder;

  UINT32 m_MatchDistances[kMatchMaxLen + 1];

  bool m_FastMode;
  bool m_MaxMode;
  UINT32 m_NumFastBytes;
  UINT32 m_LongestMatchLength;    

  UINT32 m_AdditionalOffset;

  UINT32 m_OptimumEndIndex;
  UINT32 m_OptimumCurrentIndex;

  bool m_LongestMatchWasFound;

  UINT32 m_PosSlotPrices[kNumLenToPosStates][kDistTableSizeMax];
  
  UINT32 m_DistancesPrices[kNumLenToPosStates][kNumFullDistances];

  UINT32 m_AlignPrices[kAlignTableSize];
  UINT32 m_AlignPriceCount;

  UINT32 m_DistTableSize;

  UINT32 m_PosStateBits;
  UINT32 m_PosStateMask;
  UINT32 m_LiteralPosStateBits;
  UINT32 m_LiteralContextBits;

  UINT32 m_DictionarySize;


  UINT32 m_DictionarySizePrev;
  UINT32 m_NumFastBytesPrev;

  
  UINT32 ReadMatchDistances()
  {
    UINT32 aLen = m_MatchFinder->GetLongestMatch(m_MatchDistances);
    if (aLen == m_NumFastBytes)
      aLen += m_MatchFinder->GetMatchLen(aLen, m_MatchDistances[aLen], 
          kMatchMaxLen - aLen);
    m_AdditionalOffset++;
    HRESULT aResult = m_MatchFinder->MovePos();
    if (aResult != S_OK)
      throw CMatchFinderException(aResult);
    return aLen;
  }

  void MovePos(UINT32 aNum);
  UINT32 GetRepLen1Price(CState aState, UINT32 aPosState) const
  {
    return m_MatchRepChoiceEncoders[aState.m_Index].GetPrice(0) +
        m_MatchRepShortChoiceEncoders[aState.m_Index][aPosState].GetPrice(0);
  }
  UINT32 GetRepPrice(UINT32 aRepIndex, UINT32 aLen, CState aState, UINT32 aPosState) const
  {
    UINT32 aPrice = m_RepMatchLenEncoder.GetPrice(aLen - kMatchMinLen, aPosState);
    if(aRepIndex == 0)
    {
      aPrice += m_MatchRepChoiceEncoders[aState.m_Index].GetPrice(0);
      aPrice += m_MatchRepShortChoiceEncoders[aState.m_Index][aPosState].GetPrice(1);
    }
    else
    {
      aPrice += m_MatchRepChoiceEncoders[aState.m_Index].GetPrice(1);
      if (aRepIndex == 1)
        aPrice += m_MatchRep1ChoiceEncoders[aState.m_Index].GetPrice(0);
      else
      {
        aPrice += m_MatchRep1ChoiceEncoders[aState.m_Index].GetPrice(1);
        aPrice += m_MatchRep2ChoiceEncoders[aState.m_Index].GetPrice(aRepIndex - 2);
      }
    }
    return aPrice;
  }
  /*
  UINT32 GetPosLen2Price(UINT32 aPos, UINT32 aPosState) const
  {
    if (aPos >= kNumFullDistances)
      return kIfinityPrice;
    return m_DistancesPrices[0][aPos] + m_LenEncoder.GetPrice(0, aPosState);
  }
  UINT32 GetPosLen3Price(UINT32 aPos, UINT32 aLen, UINT32 aPosState) const
  {
    UINT32 aPrice;
    UINT32 aLenToPosState = GetLenToPosState(aLen);
    if (aPos < kNumFullDistances)
      aPrice = m_DistancesPrices[aLenToPosState][aPos];
    else
      aPrice = m_PosSlotPrices[aLenToPosState][GetPosSlot2(aPos)] + 
          m_AlignPrices[aPos & kAlignMask];
    return aPrice + m_LenEncoder.GetPrice(aLen - kMatchMinLen, aPosState);
  }
  */
  UINT32 GetPosLenPrice(UINT32 aPos, UINT32 aLen, UINT32 aPosState) const
  {
    if (aLen == 2 && aPos >= 0x80)
      return kIfinityPrice;
    UINT32 aPrice;
    UINT32 aLenToPosState = GetLenToPosState(aLen);
    if (aPos < kNumFullDistances)
      aPrice = m_DistancesPrices[aLenToPosState][aPos];
    else
      aPrice = m_PosSlotPrices[aLenToPosState][GetPosSlot2(aPos)] + 
          m_AlignPrices[aPos & kAlignMask];
    return aPrice + m_LenEncoder.GetPrice(aLen - kMatchMinLen, aPosState);
  }

  UINT32 Backward(UINT32 &aBackRes, UINT32 aCur);
  UINT32 GetOptimum(UINT32 &aBackRes, UINT32 aPosition);
  UINT32 GetOptimumFast(UINT32 &aBackRes, UINT32 aPosition);

  void FillPosSlotPrices();
  void FillDistancesPrices();
  void FillAlignPrices();
    
  void ReleaseStreams()
  {
    m_MatchFinder->ReleaseStream();
    m_RangeEncoder.ReleaseStream();
  }
  HRESULT Flush();
  class CCoderReleaser
  {
    CEncoder *m_Coder;
  public:
    CCoderReleaser(CEncoder *aCoder): m_Coder(aCoder) {}
    ~CCoderReleaser()
    {
      m_Coder->ReleaseStreams();
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
                 TEXT("Compress.LZMAEncoder"), 0, THREADFLAGS_APARTMENT)


  STDMETHOD(Init)(ISequentialInStream *anInStream, 
      ISequentialOutStream *anOutStream);
  
  // ICompressCoder interface
  HRESULT CodeReal(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, 
      const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);
  
  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, 
      const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  // IInitMatchFinder interface
  STDMETHOD(InitMatchFinder)(IInWindowStreamMatch *aMatchFinder);

  // ICompressSetCoderProperties2
  STDMETHOD(SetCoderProperties2)(const PROPID *aPropIDs, 
      const PROPVARIANT *aProperties, UINT32 aNumProperties);
  
  // ICompressSetEncoderProperties2
  STDMETHOD(SetEncoderProperties2)(const PROPID *aPropIDs, 
      const PROPVARIANT *aProperties, UINT32 aNumProperties);

  // ICompressWriteCoderProperties
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *anOutStream);
};

}}

#endif
