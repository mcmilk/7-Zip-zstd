// LZArithmetic/Encoder.cpp

#include "StdAfx.h"

#include <new>

#include "Encoder.h"

#include "Windows/Defs.h"

using namespace NCompression;
using namespace NArithmetic;

#define RETURN_E_OUTOFMEMORY_IF_FALSE(x) { if (!(x)) return E_OUTOFMEMORY; }

namespace NCompress {
namespace NLZMA {

static BYTE g_FastPos[1024];

class CFastPosInit
{
public:
  CFastPosInit()
  {
    int c = 0;
    const kFastSlots = 20;
    c = 0;
    for (BYTE aSlotFast = 0; aSlotFast < kFastSlots; aSlotFast++)
    {
      UINT32 k = (1 << kDistDirectBits[aSlotFast]);
      for (UINT32 j = 0; j < k; j++, c++)
        g_FastPos[c] = aSlotFast;
    }
  }
} g_FastPosInit;

const kDefaultDictionaryLogSize = 20;
const kNumFastBytesDefault = 0x20;

CEncoder::CEncoder():
  m_DictionarySize(1 << kDefaultDictionaryLogSize),
  m_DictionarySizePrev(UINT32(-1)),
  m_NumFastBytes(kNumFastBytesDefault),
  m_NumFastBytesPrev(UINT32(-1)),
  m_DistTableSize(kDefaultDictionaryLogSize * 2),
  m_PosStateBits(2),
  m_PosStateMask(4 - 1),
  m_LiteralPosStateBits(0),
  m_LiteralContextBits(3)
{
  m_PosAlignEncoder.Create(kNumAlignBits);
  for(int i = 0; i < kNumPosModels; i++)
    m_PosEncoders[i].Create(kDistDirectBits[kStartPosModelIndex + i]);
}

HRESULT CEncoder::Create()
{
  if (m_DictionarySize == m_DictionarySizePrev && m_NumFastBytesPrev == m_NumFastBytes)
    return S_OK;
  RETURN_IF_NOT_S_OK(m_MatchFinder->Create(m_DictionarySize, kNumOpts, m_NumFastBytes, 
      kMatchMaxLen - m_NumFastBytes));
  m_DictionarySizePrev = m_DictionarySize;
  m_NumFastBytesPrev = m_NumFastBytes;
  m_LiteralEncoder.Create(m_LiteralPosStateBits, m_LiteralContextBits);
  m_LenEncoder.Create(1 << m_PosStateBits);
  m_RepMatchLenEncoder.Create(1 << m_PosStateBits);
  return S_OK;
}


// ICompressSetEncoderProperties2
STDMETHODIMP CEncoder::SetEncoderProperties2(const PROPID *aPropIDs, 
    const PROPVARIANT *aProperties, UINT32 aNumProperties)
{
  if (aNumProperties != 1)
    return E_INVALIDARG;
  if (aPropIDs[0] != NEncodingProperies::kNumFastBytes)
    return E_INVALIDARG;
  if (aProperties[0].vt != VT_UI4)
    return E_INVALIDARG;

  UINT32 aNumFastBytes = aProperties[0].ulVal;
  if(aNumFastBytes < 2 || aNumFastBytes > kMatchMaxLen)
     return E_INVALIDARG;
  m_NumFastBytes = aNumFastBytes;

  return S_OK;
}

// ICompressSetCoderProperties
STDMETHODIMP CEncoder::SetCoderProperties2(const PROPID *aPropIDs, 
    const PROPVARIANT *aProperties, UINT32 aNumProperties)
{
  for (UINT32 i = 0; i < aNumProperties; i++)
  {
    const PROPVARIANT &aProperty = aProperties[i];
    switch(aPropIDs[i])
    {
      case NEncodedStreamProperies::kDictionarySize:
      {
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 aDictionarySize = aProperty.ulVal;
        if (aDictionarySize > UINT32(1 << kDicLogSizeMax))
          return E_INVALIDARG;
        m_DictionarySize = aDictionarySize;
        UINT32 aDicLogSize;
        for(aDicLogSize = 0; aDicLogSize < kDicLogSizeMax; aDicLogSize++)
          if (aDictionarySize <= (1 << aDicLogSize))
            break;
        m_DistTableSize = aDicLogSize * 2;
        break;
      }
      case NEncodedStreamProperies::kPosStateBits:
      {
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 aValue = aProperty.ulVal;
        if (aValue > NLength::kNumPosStatesBitsEncodingMax)
          return E_INVALIDARG;
        m_PosStateBits = aValue;
        m_PosStateMask = (1 << m_PosStateBits) - 1;
        break;
      }
      case NEncodedStreamProperies::kLitPosBits:
      {
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 aValue = aProperty.ulVal;
        if (aValue > kNumLitPosStatesBitsEncodingMax)
          return E_INVALIDARG;
        m_LiteralPosStateBits = aValue;
        break;
      }
      case NEncodedStreamProperies::kLitContextBits:
      {
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 aValue = aProperty.ulVal;
        if (aValue > kNumLitContextBitsMax)
          return E_INVALIDARG;
        m_LiteralContextBits = aValue;
        break;
      }
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *anOutStream)
{ 
  BYTE aByte = (m_PosStateBits * 5 + m_LiteralPosStateBits) * 9 + m_LiteralContextBits;
  RETURN_IF_NOT_S_OK(anOutStream->Write(&aByte, sizeof(aByte), NULL));
  return anOutStream->Write(&m_DictionarySize, sizeof(m_DictionarySize), NULL);
}


STDMETHODIMP CEncoder::Init(ISequentialInStream *anInStream, 
    ISequentialOutStream *anOutStream)
{
  CBaseCoder::Init();

  RETURN_IF_NOT_S_OK(m_MatchFinder->Init(anInStream));
  m_RangeEncoder.Init(anOutStream);

  for(int i = 0; i < kNumStates; i++)
  {
    for (UINT32 j = 0; j <= m_PosStateMask; j++)
    {
      m_MainChoiceEncoders[i][j].Init();
      m_MatchRepShortChoiceEncoders[i][j].Init();
    }
    m_MatchChoiceEncoders[i].Init();
    m_MatchRepChoiceEncoders[i].Init();
    m_MatchRep1ChoiceEncoders[i].Init();
    m_MatchRep2ChoiceEncoders[i].Init();
  }

  m_LiteralEncoder.Init();

  // m_RepMatchLenEncoder.Init();
  
  for(i = 0; i < kNumLenToPosStates; i++)
    m_PosSlotEncoder[i].Init();

  for(i = 0; i < kNumPosModels; i++)
    m_PosEncoders[i].Init();

  m_LenEncoder.Init();
  m_RepMatchLenEncoder.Init();

  m_PosAlignEncoder.Init();

  m_LongestMatchWasFound = false;
  m_OptimumEndIndex = 0;
  m_OptimumCurrentIndex = 0;
  m_AdditionalOffset = 0;

  return S_OK;
}

void CEncoder::MovePos(UINT32 aNum)
{
  for (;aNum > 0; aNum--)
  {
    m_MatchFinder->DummyLongestMatch();
    HRESULT aResult = m_MatchFinder->MovePos();
    if (aResult != S_OK)
      throw CMatchFinderException(aResult);
    m_AdditionalOffset++;
  }
}

UINT32 CEncoder::Backward(UINT32 &aBackRes, UINT32 aCur)
{
  m_OptimumEndIndex = aCur;
  UINT32 aPosMem = m_Optimum[aCur].PosPrev;
  UINT32 aBackMem = m_Optimum[aCur].BackPrev;
  do
  {
    UINT32 aPosPrev = aPosMem;
    UINT32 aBackCur = aBackMem;

    aBackMem = m_Optimum[aPosPrev].BackPrev;
    aPosMem = m_Optimum[aPosPrev].PosPrev;

    m_Optimum[aPosPrev].BackPrev = aBackCur;
    m_Optimum[aPosPrev].PosPrev = aCur;
    aCur = aPosPrev;
  }
  while(aCur > 0);
  aBackRes = m_Optimum[0].BackPrev;
  m_OptimumCurrentIndex  = m_Optimum[0].PosPrev;
  return m_OptimumCurrentIndex; 
}

/*
inline UINT32 GetMatchLen(const BYTE *aData, UINT32 aBack, UINT32 aLimit)
{  
  aBack++;
  for(UINT32 i = 0; i < aLimit && aData[i] == aData[i - aBack]; i++);
  return i;
}
*/

UINT32 CEncoder::GetOptimum(UINT32 &aBackRes, UINT32 aPosition)
{
  if(m_OptimumEndIndex != m_OptimumCurrentIndex)
  {
    UINT32 aLen = m_Optimum[m_OptimumCurrentIndex].PosPrev - m_OptimumCurrentIndex;
    aBackRes = m_Optimum[m_OptimumCurrentIndex].BackPrev;
    m_OptimumCurrentIndex = m_Optimum[m_OptimumCurrentIndex].PosPrev;
    return aLen;
  }
  m_OptimumCurrentIndex = 0;
  m_OptimumEndIndex = 0; // test it;
  
  UINT32 aLenMain;
  if (!m_LongestMatchWasFound)
    aLenMain = ReadMatchDistances();
  else
  {
    aLenMain = m_LongestMatchLength;
    m_LongestMatchWasFound = false;
  }


  UINT32 aReps[kNumRepDistances];
  UINT32 aRepLens[kNumRepDistances];
  UINT32 RepMaxIndex;
  for(int i = 0; i < kNumRepDistances; i++)
  {
    aReps[i] = m_RepDistances[i];
    aRepLens[i] = m_MatchFinder->GetMatchLen(0 - 1, aReps[i], kMatchMaxLen);
    if (i == 0 || aRepLens[i] > aRepLens[RepMaxIndex])
      RepMaxIndex = i;
  }
  if(aRepLens[RepMaxIndex] > m_NumFastBytes)
  {
    aBackRes = RepMaxIndex;
    MovePos(aRepLens[RepMaxIndex] - 1);
    return aRepLens[RepMaxIndex];
  }

  if(aLenMain > m_NumFastBytes)
  {
    UINT32 aBackMain = (aLenMain < m_NumFastBytes) ? m_MatchDistances[aLenMain] :
        m_MatchDistances[m_NumFastBytes];
    aBackRes = aBackMain + kNumRepDistances; 
    MovePos(aLenMain - 1);
    return aLenMain;
  }
  BYTE aCurrentByte = m_MatchFinder->GetIndexByte(0 - 1);

  m_Optimum[0].State = m_State;

  BYTE aMatchByte;
  
  aMatchByte = m_MatchFinder->GetIndexByte(0 - m_RepDistances[0] - 1 - 1);

  UINT32 aPosState = (aPosition & m_PosStateMask);

  m_Optimum[1].Price = m_MainChoiceEncoders[m_State.m_Index][aPosState].GetPrice(kMainChoiceLiteralIndex) + 
      m_LiteralEncoder.GetPrice(aPosition, m_PreviousByte, m_PeviousIsMatch, aMatchByte, aCurrentByte);
  m_Optimum[1].MakeAsChar();

  BYTE aPreviousByteLocal = aCurrentByte;
  m_Optimum[1].PosPrev = 0;

  for (i = 0; i < kNumRepDistances; i++)
    m_Optimum[0].Backs[i] = aReps[i];

  UINT32 aMatchPrice = m_MainChoiceEncoders[m_State.m_Index][aPosState].GetPrice(kMainChoiceMatchIndex);
  UINT32 aRepMatchPrice = aMatchPrice + 
      m_MatchChoiceEncoders[m_State.m_Index].GetPrice(kMatchChoiceRepetitionIndex);

  if(aMatchByte == aCurrentByte)
  {
    UINT32 aShortRepPrice = aRepMatchPrice + GetRepLen1Price(m_State, aPosState);
    if(aShortRepPrice < m_Optimum[1].Price)
    {
      m_Optimum[1].Price = aShortRepPrice;
      m_Optimum[1].MakeAsShortRep();
    }
  }
  if(aLenMain < 2)
  {
    aBackRes = m_Optimum[1].BackPrev;
    return 1;
  }

  
  UINT32 aNormalMatchPrice = aMatchPrice + 
      m_MatchChoiceEncoders[m_State.m_Index].GetPrice(kMatchChoiceDistanceIndex);

  if (aLenMain <= aRepLens[RepMaxIndex])
    aLenMain = 0;

  UINT32 aLen;
  for(aLen = 2; aLen <= aLenMain; aLen++)
  {
    m_Optimum[aLen].PosPrev = 0;
    m_Optimum[aLen].BackPrev = m_MatchDistances[aLen] + kNumRepDistances;
    m_Optimum[aLen].Price = aNormalMatchPrice + 
        GetPosLenPrice(m_MatchDistances[aLen], aLen, aPosState);
  }

  if (aLenMain < aRepLens[RepMaxIndex])
    aLenMain = aRepLens[RepMaxIndex];

  for (; aLen <= aLenMain; aLen++)
    m_Optimum[aLen].Price = kIfinityPrice;

  for(i = 0; i < kNumRepDistances; i++)
  {
    UINT aRepLen = aRepLens[i];
    for(UINT32 aLenTest = 2; aLenTest <= aRepLen; aLenTest++)
    {
      UINT32 aCurAndLenPrice = aRepMatchPrice + GetRepPrice(i, aLenTest, m_State, aPosState);
      COptimal &anOptimum = m_Optimum[aLenTest];
      if (aCurAndLenPrice < anOptimum.Price) 
      {
        anOptimum.Price = aCurAndLenPrice;
        anOptimum.PosPrev = 0;
        anOptimum.BackPrev = i;
      }
    }
  }

  UINT32 aCur = 0;
  UINT32 aLenEnd = aLenMain;

  while(true)
  {
    aCur++;
    aPosition++;
    UINT32 aPosPrev  = m_Optimum[aCur].PosPrev;
    CState aState = m_Optimum[aPosPrev].State;

    bool aPrevWasMatch;
    if (aPosPrev == aCur - 1)
    {
      if (m_Optimum[aCur].IsShortRep())
      {
        aPrevWasMatch = true;
        aState.UpdateShortRep();
      }
      else
      {
        aPrevWasMatch = false;
        aState.UpdateChar();
      }
    }
    else
    {
      aPrevWasMatch = true;
      UINT32 aPos = m_Optimum[aCur].BackPrev;
      if (aPos < kNumRepDistances)
      {
        aState.UpdateRep();
        aReps[0] = m_Optimum[aPosPrev].Backs[aPos];
        for(UINT32 i = 1; i <= aPos; i++)
          aReps[i] = m_Optimum[aPosPrev].Backs[i - 1];
        for(; i < kNumRepDistances; i++)
          aReps[i] = m_Optimum[aPosPrev].Backs[i];
      }
      else
      {
        aState.UpdateMatch();
        aReps[0] = (aPos - kNumRepDistances);
        for(UINT32 i = 1; i < kNumRepDistances; i++)
          aReps[i] = m_Optimum[aPosPrev].Backs[i - 1];
      }
    }
    m_Optimum[aCur].State = aState;
    for(UINT32 i = 0; i < kNumRepDistances; i++)
      m_Optimum[aCur].Backs[i] = aReps[i];
    if(aCur == aLenEnd)  
      return Backward(aBackRes, aCur);
    UINT32 aNewLen = ReadMatchDistances();
    if(aNewLen > m_NumFastBytes)
    {
      m_LongestMatchLength = aNewLen;
      m_LongestMatchWasFound = true;
      return Backward(aBackRes, aCur);
    }
    UINT32 aCurPrice = m_Optimum[aCur].Price; 
    // BYTE aCurrentByte  = m_MatchFinder->GetIndexByte(0 - 1);
    // BYTE aMatchByte = m_MatchFinder->GetIndexByte(0 - aReps[0] - 1 - 1);
    const BYTE *aData = m_MatchFinder->GetPointerToCurrentPos() - 1;
    BYTE aCurrentByte = *aData;
    BYTE aMatchByte = aData[0 - aReps[0] - 1];

    UINT32 aPosState = (aPosition & m_PosStateMask);

    UINT32 aCurAnd1Price = aCurPrice +
        m_MainChoiceEncoders[aState.m_Index][aPosState].GetPrice(kMainChoiceLiteralIndex) +
        m_LiteralEncoder.GetPrice(aPosition, aPreviousByteLocal, aPrevWasMatch, aMatchByte, aCurrentByte);

    aPreviousByteLocal = aCurrentByte ;

    COptimal &aNextOptimum = m_Optimum[aCur + 1];

    if (aCurAnd1Price < aNextOptimum.Price) 
    {
      aNextOptimum.Price = aCurAnd1Price;
      aNextOptimum.PosPrev = aCur;
      aNextOptimum.MakeAsChar();
    }

    UINT32 aMatchPrice = aCurPrice + m_MainChoiceEncoders[aState.m_Index][aPosState].GetPrice(kMainChoiceMatchIndex);
    UINT32 aRepMatchPrice = aMatchPrice + m_MatchChoiceEncoders[aState.m_Index].GetPrice(kMatchChoiceRepetitionIndex);
    
    if(aMatchByte == aCurrentByte &&
        !(aNextOptimum.PosPrev < aCur && aNextOptimum.BackPrev == 0))
    {
      UINT32 aShortRepPrice = aRepMatchPrice + GetRepLen1Price(aState, aPosState);
      if(aShortRepPrice <= aNextOptimum.Price)
      {
        aNextOptimum.Price = aShortRepPrice;
        aNextOptimum.PosPrev = aCur;
        aNextOptimum.MakeAsShortRep();
      }
    }
    /*
    if(aNewLen == 2 && m_MatchDistances[2] >= kDistLimit2) // test it maybe set 2000 ?
      continue;
    */
    if(aNewLen >= 2)
    {
      if(aCur + aNewLen > aLenEnd)
      {
        if (aCur + aNewLen > kNumOpts - 1)
          aNewLen = kNumOpts - 1 - aCur;
        UINT32 aLenEndNew = aCur + aNewLen;
        if (aLenEnd < aLenEndNew)
        {
          for(UINT32 i = aLenEnd + 1; i <= aLenEndNew; i++)
            m_Optimum[i].Price = kIfinityPrice;
          aLenEnd = aLenEndNew;
        }
      }       
    }

    UINT32 aNumAvailableBytes = m_MatchFinder->GetNumAvailableBytes() + 1;
    if (aNumAvailableBytes < 2)
      continue;
    if (aNumAvailableBytes > m_NumFastBytes)
      aNumAvailableBytes = m_NumFastBytes;
    {
      // UINT32 aRepLen = m_MatchFinder->GetMatchLen(0 - 1, aReps[i], aNewLen); // test it;
      UINT32 aBackOffset = aReps[0] + 1;
      if (aData[0] == aData[0 - aBackOffset] &&
          aData[1] == aData[1 - aBackOffset])
      {
        for(UINT32 aLenTest = 2; aLenTest <= aNumAvailableBytes; aLenTest++)
        {
          UINT32 aLenEndNew = aCur + aLenTest;
          if(aLenEndNew > aLenEnd)
          {
            if (aLenEndNew > kNumOpts - 1)
              break;
            aLenEnd = aLenEndNew;
            m_Optimum[aLenEndNew].Price = kIfinityPrice;
          }
          UINT32 aCurAndLenPrice = aRepMatchPrice + GetRepPrice(0, aLenTest, aState, aPosState);
          // UINT32 aCurAndLenPrice = aPrice + m_RepLenPrices[aLenTest];
          COptimal &anOptimum = m_Optimum[aLenEndNew];
          if (aCurAndLenPrice < anOptimum.Price) 
          {
            anOptimum.Price = aCurAndLenPrice;
            anOptimum.PosPrev = aCur;
            anOptimum.BackPrev = 0;
          }
          if (aData[aLenTest] != aData[aLenTest - aBackOffset])
            break;
        }
      }
    }
    for(i = 1; i < kNumRepDistances; i++)
    {
      // UINT32 aRepLen = m_MatchFinder->GetMatchLen(0 - 1, aReps[i], aNewLen); // test it;
      UINT32 aBackOffset = aReps[i] + 1;
      if (aData[0] != aData[0 - aBackOffset] ||
          aData[1] != aData[1 - aBackOffset])
        continue;
      for(UINT32 aLenTest = 2; aLenTest <= aNumAvailableBytes; aLenTest++)
      {
        UINT32 aLenEndNew = aCur + aLenTest;
        if(aLenEndNew > aLenEnd)
        {
          if (aLenEndNew > kNumOpts - 1)
            break;
          aLenEnd = aLenEndNew;
          m_Optimum[aLenEndNew].Price = kIfinityPrice;
        }
        UINT32 aCurAndLenPrice = aRepMatchPrice + GetRepPrice(i, aLenTest, aState, aPosState);
        COptimal &anOptimum = m_Optimum[aCur + aLenTest];
        if (aCurAndLenPrice < anOptimum.Price) 
        {
          anOptimum.Price = aCurAndLenPrice;
          anOptimum.PosPrev = aCur;
          anOptimum.BackPrev = i;
        }
        if (aData[aLenTest] != aData[aLenTest - aBackOffset])
          break;
      }
    }
    
    //    for(UINT32 aLenTest = 2; aLenTest <= aNewLen; aLenTest++)
    UINT32 aNormalMatchPrice = aMatchPrice + 
        m_MatchChoiceEncoders[aState.m_Index].GetPrice(kMatchChoiceDistanceIndex);

    if (aNewLen >= 2)
    {
      UINT32 aCurBack = m_MatchDistances[2];
      UINT32 aCurAndLenPrice = aNormalMatchPrice + GetPosLen2Price(aCurBack, aPosState);
      COptimal &anOptimum = m_Optimum[aCur + 2];
      if (aCurAndLenPrice < anOptimum.Price) 
      {
        anOptimum.Price = aCurAndLenPrice;
        anOptimum.PosPrev = aCur;
        anOptimum.BackPrev = aCurBack + kNumRepDistances;
      }
      for(UINT32 aLenTest = 3; aLenTest <= aNewLen; aLenTest++)
      {
        UINT32 aCurBack = m_MatchDistances[aLenTest];
        UINT32 aCurAndLenPrice = aNormalMatchPrice + GetPosLen3Price(aCurBack, aLenTest, aPosState);
        COptimal &anOptimum = m_Optimum[aCur + aLenTest];
        if (aCurAndLenPrice < anOptimum.Price) 
        {
          anOptimum.Price = aCurAndLenPrice;
          anOptimum.PosPrev = aCur;
          anOptimum.BackPrev = aCurBack + kNumRepDistances;
        }
      }
    }
  }
}


STDMETHODIMP CEncoder::InitMatchFinder(IInWindowStreamMatch *aMatchFinder)
{
  m_MatchFinder = aMatchFinder;
  return S_OK;
}

HRESULT CEncoder::Flush()
{
  m_RangeEncoder.FlushData();
  return m_RangeEncoder.FlushStream();
}

HRESULT CEncoder::CodeReal(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, 
      const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress)
{
  RETURN_IF_NOT_S_OK(Create());
  Init(anInStream, anOutStream);
  CCoderReleaser aReleaser(this);

  if (m_MatchFinder->GetNumAvailableBytes() == 0)
    return Flush();

  FillPosSlotPrices();
  FillDistancesPrices();
  FillAlignPrices();

  m_LenEncoder.SetTableSize(m_NumFastBytes);
  m_LenEncoder.UpdateTables();
  m_RepMatchLenEncoder.SetTableSize(m_NumFastBytes);
  m_RepMatchLenEncoder.UpdateTables();

  UINT64 aLastPosSlotFillingPos = 0;

  UINT64 aProgressPosValuePrev = 0;
  UINT64 aNowPos64 = 0;
  ReadMatchDistances();
  UINT32 aPosState = aNowPos64 & m_PosStateMask;
  m_MainChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, kMainChoiceLiteralIndex);
  m_State.UpdateChar();
  BYTE aByte = m_MatchFinder->GetIndexByte(0 - m_AdditionalOffset);
  m_LiteralEncoder.Encode(&m_RangeEncoder, UINT32(aNowPos64), m_PreviousByte, false, 0, aByte);
  m_PreviousByte = aByte;
  m_AdditionalOffset--;
  aNowPos64++;
  if (m_MatchFinder->GetNumAvailableBytes() == 0)
    return Flush();
  while(true)
  {
    UINT32 aPos;
    UINT32 aPosState = aNowPos64 & m_PosStateMask;

    UINT32 aLen = GetOptimum(aPos, aNowPos64);

    if(aLen == 1 && aPos == (-1))
    {
      m_MainChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, kMainChoiceLiteralIndex);
      m_State.UpdateChar();
      BYTE aMatchByte;
      if(m_PeviousIsMatch)
        aMatchByte = m_MatchFinder->GetIndexByte(0 - m_RepDistances[0] - 1 - m_AdditionalOffset);
      BYTE aByte = m_MatchFinder->GetIndexByte(0 - m_AdditionalOffset);
      m_LiteralEncoder.Encode(&m_RangeEncoder, UINT32(aNowPos64), m_PreviousByte, m_PeviousIsMatch, aMatchByte, aByte);
      m_PreviousByte = aByte;
      m_PeviousIsMatch = false;
    }
    else
    {
      m_PeviousIsMatch = true;
      m_MainChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, kMainChoiceMatchIndex);
      if(aPos < kNumRepDistances)
      {
        m_MatchChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, kMatchChoiceRepetitionIndex);
        if(aPos == 0)
        {
          m_MatchRepChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, 0);
          if(aLen == 1)
            m_MatchRepShortChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, 0);
          else
            m_MatchRepShortChoiceEncoders[m_State.m_Index][aPosState].Encode(&m_RangeEncoder, 1);
        }
        else
        {
          m_MatchRepChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, 1);
          if (aPos == 1)
            m_MatchRep1ChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, 0);
          else
          {
            m_MatchRep1ChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, 1);
            m_MatchRep2ChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, aPos - 2);
          }
        }
        if (aLen == 1)
          m_State.UpdateShortRep();
        else
        {
          m_RepMatchLenEncoder.Encode(&m_RangeEncoder, aLen - kMatchMinLen, aPosState);
          m_State.UpdateRep();
        }


        UINT32 aDistance = m_RepDistances[aPos];
        if (aPos != 0)
        {
          for(UINT32 i = aPos; i >= 1; i--)
            m_RepDistances[i] = m_RepDistances[i - 1];
          m_RepDistances[0] = aDistance;
        }
      }
      else
      {
        m_MatchChoiceEncoders[m_State.m_Index].Encode(&m_RangeEncoder, kMatchChoiceDistanceIndex);
        m_State.UpdateMatch();
        m_LenEncoder.Encode(&m_RangeEncoder, aLen - kMatchMinLen, aPosState);
        aPos -= kNumRepDistances;
        UINT32 aPosSlot = GetPosSlot(aPos);
        UINT32 aLenToPosState = GetLenToPosState(aLen);
        m_PosSlotEncoder[aLenToPosState].Encode(&m_RangeEncoder, aPosSlot);
        
        UINT32 aFooterBits = kDistDirectBits[aPosSlot];
        UINT32 aPosReduced = aPos - kDistStart[aPosSlot];
        if (aPosSlot >= kStartPosModelIndex)
        {
          if (aPosSlot < kEndPosModelIndex)
            m_PosEncoders[aPosSlot - kStartPosModelIndex].Encode(&m_RangeEncoder, aPosReduced);
          else
          {
            m_RangeEncoder.EncodeDirectBits(aPosReduced >> kNumAlignBits, aFooterBits - kNumAlignBits);
            m_PosAlignEncoder.Encode(&m_RangeEncoder, aPosReduced & kAlignMask);
            if (--m_AlignPriceCount == 0)
              FillAlignPrices();
          }
        }
        UINT32 aDistance = aPos;
        for(UINT32 i = kNumRepDistances - 1; i >= 1; i--)
          m_RepDistances[i] = m_RepDistances[i - 1];
        m_RepDistances[0] = aDistance;
      }
      m_PreviousByte = m_MatchFinder->GetIndexByte(aLen - 1 - m_AdditionalOffset);
    }
    m_AdditionalOffset -= aLen;
    aNowPos64 += aLen;
    if (aNowPos64 - aLastPosSlotFillingPos >= (1 << 9))
    {
      FillPosSlotPrices();
      FillDistancesPrices();
      aLastPosSlotFillingPos = aNowPos64;
    }
    if (aNowPos64 - aProgressPosValuePrev >= (1 << 12) && aProgress != NULL)
    {
      UINT64 anOutSize = m_RangeEncoder.GetProcessedSize();
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&aNowPos64, &anOutSize));
      aProgressPosValuePrev = aNowPos64;
    }
    if (m_AdditionalOffset == 0 && m_MatchFinder->GetNumAvailableBytes() == 0)
      return Flush();
  }
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  try
  {
    return CodeReal(anInStream, anOutStream, anInSize, anOutSize, aProgress);
  }
  catch(CMatchFinderException &anException)
  {
    return anException.m_Result;
  }
  catch(const NStream::COutByteWriteException &anException)
  {
    return anException.m_Result;
  }
  catch(...)
  {
    return E_FAIL;
  }
}
  
void CEncoder::FillPosSlotPrices()
{
  for (int aLenToPosState = 0; aLenToPosState < kNumLenToPosStates; aLenToPosState++)
  {
    for (int aPosSlot = 0; aPosSlot < kEndPosModelIndex && aPosSlot < m_DistTableSize; aPosSlot++)
      m_PosSlotPrices[aLenToPosState][aPosSlot] = m_PosSlotEncoder[aLenToPosState].GetPrice(aPosSlot);
    for (; aPosSlot < m_DistTableSize; aPosSlot++)
      m_PosSlotPrices[aLenToPosState][aPosSlot] = m_PosSlotEncoder[aLenToPosState].GetPrice(aPosSlot) + 
          ((kDistDirectBits[aPosSlot] - kNumAlignBits) << kNumBitPriceShiftBits);
  }
}

void CEncoder::FillDistancesPrices()
{
  for (int aLenToPosState = 0; aLenToPosState < kNumLenToPosStates; aLenToPosState++)
  {
    for (UINT32 i = 0; i < kStartPosModelIndex; i++)
      m_DistancesPrices[aLenToPosState][i] = m_PosSlotPrices[aLenToPosState][i];
    for (; i < kNumFullDistances; i++)
    { 
      UINT32 aPosSlot = GetPosSlot(i);
      m_DistancesPrices[aLenToPosState][i] = m_PosSlotPrices[aLenToPosState][aPosSlot] +
          m_PosEncoders[aPosSlot - kStartPosModelIndex].GetPrice(i - kDistStart[aPosSlot]);
    }
  }
}

void CEncoder::FillAlignPrices()
{
  for (int i = 0; i < kAlignTableSize; i++)
    m_AlignPrices[i] = m_PosAlignEncoder.GetPrice(i);
  m_AlignPriceCount = kAlignTableSize;
}

}}