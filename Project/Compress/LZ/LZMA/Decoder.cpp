// Decoder.cpp

#include "StdAfx.h"

#include "Decoder.h"
#include "CoderInfo.h"

#include "Common/Defs.h"

#include "Windows/Defs.h"

/*
#include "fstream.h"
#include "iomanip.h"

ofstream ofs("res.dat");

UINT32 GetTimeCount()
{
  unsigned int aValueLow;
  __asm RDTSC;
  __asm mov aValueLow, EAX;
  return aValueLow;
}

const kNumCounters = 3;
UINT32 g_Counter[kNumCounters];
class C1
{
public:
  ~C1()
  {
    for (int i = 0; i < kNumCounters; i++)
      ofs << setw(10) << g_Counter[i] << endl;
  }
} g_C1;
*/

/*
const UINT32 kLenTableMax = 20;
const UINT32 kNumDists = NCompress::NLZMA::kDistTableSizeMax / 2;
UINT32 g_Counts[kLenTableMax][kNumDists];
class C1
{
public:
  ~C1 ()
  {
    UINT32 aSums[kLenTableMax];
    for (int aLen = 2; aLen < kLenTableMax; aLen++)
    {
      aSums[aLen] = 0;
      for (int aDist = 0; aDist < kNumDists; aDist++)
        aSums[aLen] += g_Counts[aLen][aDist];
      if (aSums[aLen] == 0)
        aSums[aLen] = 1;
    }
    for (int aDist = 0; aDist < kNumDists; aDist++)
    {
      ofs << setw(4) << aDist << "  ";
      for (int aLen = 2; aLen < kLenTableMax; aLen++)
      {
        ofs << setw(4) << g_Counts[aLen][aDist] * 1000 / aSums[aLen];
      }
      ofs << endl;
    }
  }
} g_Class;

void UpdateStat(UINT32 aLen, UINT32 aDist)
{
  if (aLen >= kLenTableMax)
    aLen = kLenTableMax - 1;
  g_Counts[aLen][aDist / 2]++;
}
*/

#define RETURN_E_OUTOFMEMORY_IF_FALSE(x) { if (!(x)) return E_OUTOFMEMORY; }

namespace NCompress {
namespace NLZMA {

HRESULT CDecoder::SetDictionarySize(UINT32 aDictionarySize)
{
  if (aDictionarySize > (1 << kDicLogSizeMax))
    return E_INVALIDARG;
  
  UINT32 aWindowReservSize = MyMax(aDictionarySize, UINT32(1 << 21));

  if (m_DictionarySize != aDictionarySize)
  {
    m_OutWindowStream.Create(aDictionarySize, kMatchMaxLen, aWindowReservSize);
    m_DictionarySize = aDictionarySize;
  }
  return S_OK;
}

HRESULT CDecoder::SetLiteralProperties(
    UINT32 aLiteralPosStateBits, UINT32 aLiteralContextBits)
{
  if (aLiteralPosStateBits > 8)
    return E_INVALIDARG;
  if (aLiteralContextBits > 8)
    return E_INVALIDARG;
  m_LiteralDecoder.Create(aLiteralPosStateBits, aLiteralContextBits);
  return S_OK;
}

HRESULT CDecoder::SetPosBitsProperties(UINT32 aNumPosStateBits)
{
  if (aNumPosStateBits > NLength::kNumPosStatesBitsMax)
    return E_INVALIDARG;
  UINT32 aNumPosStates = 1 << aNumPosStateBits;
  m_LenDecoder.Create(aNumPosStates);
  m_RepMatchLenDecoder.Create(aNumPosStates);
  m_PosStateMask = aNumPosStates - 1;
  return S_OK;
}

CDecoder::CDecoder():
  m_DictionarySize(-1)
{
  Create();
}

HRESULT CDecoder::Create()
{
  for(int i = 0; i < kNumPosModels; i++)
  {
    RETURN_E_OUTOFMEMORY_IF_FALSE(
        m_PosDecoders[i].Create(kDistDirectBits[kStartPosModelIndex + i]));
  }
  return S_OK;
}


HRESULT CDecoder::Init(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream)
{
  m_RangeDecoder.Init(anInStream);

  m_OutWindowStream.Init(anOutStream);

  for(int i = 0; i < kNumStates; i++)
  {
    for (UINT32 j = 0; j <= m_PosStateMask; j++)
    {
      m_MainChoiceDecoders[i][j].Init();
      m_MatchRepShortChoiceDecoders[i][j].Init();
    }
    m_MatchChoiceDecoders[i].Init();
    m_MatchRepChoiceDecoders[i].Init();
    m_MatchRep1ChoiceDecoders[i].Init();
    m_MatchRep2ChoiceDecoders[i].Init();
  }
  
  m_LiteralDecoder.Init();
   
  // m_RepMatchLenDecoder.Init();

  for (i = 0; i < kNumLenToPosStates; i++)
    m_PosSlotDecoder[i].Init();

  for(i = 0; i < kNumPosModels; i++)
    m_PosDecoders[i].Init();
  
  m_LenDecoder.Init();
  m_RepMatchLenDecoder.Init();

  m_PosAlignDecoder.Init();
  return S_OK;

}



STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, 
    const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  if (anOutSize == NULL)
    return E_INVALIDARG;

  Init(anInStream, anOutStream);
  CDecoderFlusher aFlusher(this);

  CState aState;
  aState.Init();
  bool aPeviousIsMatch = false;
  BYTE aPreviousByte = 0;
  UINT32 aRepDistances[kNumRepDistances];
  for(int i = 0 ; i < kNumRepDistances; i++)
    aRepDistances[i] = 0;

  UINT64 aProgressPosValuePrev = 0;
  UINT64 aNowPos64 = 0;
  UINT64 aSize = *anOutSize;
  while(aNowPos64 < aSize)
  {
    const UINT64 kStepSize = 1 << 18;
    UINT64 aNext = (UINT32)MyMin(aNowPos64 + kStepSize, aSize);
    
    while(aNowPos64 < aNext)
    {
      UINT32 aPosState = UINT32(aNowPos64) & m_PosStateMask;
      if (m_MainChoiceDecoders[aState.m_Index][aPosState].Decode(&m_RangeDecoder) == kMainChoiceLiteralIndex)
      {
        // aCounts[0]++;
        aState.UpdateChar();
        if(aPeviousIsMatch)
        {
          BYTE aMatchByte = m_OutWindowStream.GetOneByte(0 - aRepDistances[0] - 1);
          aPreviousByte = m_LiteralDecoder.DecodeWithMatchByte(&m_RangeDecoder, 
              UINT32(aNowPos64), aPreviousByte, aMatchByte);
          aPeviousIsMatch = false;
        }
        else
          aPreviousByte = m_LiteralDecoder.DecodeNormal(&m_RangeDecoder, 
              UINT32(aNowPos64), aPreviousByte);
        m_OutWindowStream.PutOneByte(aPreviousByte);
        aNowPos64++;
      }
      else             
      {
        aPeviousIsMatch = true;
        UINT32 aDistance, aLen;
        if(m_MatchChoiceDecoders[aState.m_Index].Decode(&m_RangeDecoder) == 
            kMatchChoiceRepetitionIndex)
        {
          if(m_MatchRepChoiceDecoders[aState.m_Index].Decode(&m_RangeDecoder) == 0)
          {
            if(m_MatchRepShortChoiceDecoders[aState.m_Index][aPosState].Decode(&m_RangeDecoder) == 0)
            {
              aState.UpdateShortRep();
              aPreviousByte = m_OutWindowStream.GetOneByte(0 - aRepDistances[0] - 1);
              m_OutWindowStream.PutOneByte(aPreviousByte);
              aNowPos64++;
              // aCounts[3 + 4]++;
              continue;
            }
            // aCounts[3 + 0]++;
            aDistance = aRepDistances[0];
          }
          else
          {
            if(m_MatchRep1ChoiceDecoders[aState.m_Index].Decode(&m_RangeDecoder) == 0)
            {
              aDistance = aRepDistances[1];
              aRepDistances[1] = aRepDistances[0];
              // aCounts[3 + 1]++;
            }
            else 
            {
              if (m_MatchRep2ChoiceDecoders[aState.m_Index].Decode(&m_RangeDecoder) == 0)
              {
                // aCounts[3 + 2]++;
                aDistance = aRepDistances[2];
              }
              else
              {
                // aCounts[3 + 3]++;
                aDistance = aRepDistances[3];
                aRepDistances[3] = aRepDistances[2];
              }
              aRepDistances[2] = aRepDistances[1];
              aRepDistances[1] = aRepDistances[0];
            }
            aRepDistances[0] = aDistance;
          }
          aLen = m_RepMatchLenDecoder.Decode(&m_RangeDecoder, aPosState) + kMatchMinLen;
          // aCounts[aLen]++;
          aState.UpdateRep();
        }
        else
        {
          aLen = kMatchMinLen + m_LenDecoder.Decode(&m_RangeDecoder, aPosState);
          aState.UpdateMatch();
          UINT32 aPosSlot = m_PosSlotDecoder[GetLenToPosState(aLen)].Decode(&m_RangeDecoder);
          // aCounts[aPosSlot]++;
          if (aPosSlot >= kStartPosModelIndex)
          {
            aDistance = kDistStart[aPosSlot];
            if (aPosSlot < kEndPosModelIndex)
              aDistance += m_PosDecoders[aPosSlot - kStartPosModelIndex].Decode(&m_RangeDecoder);
            else
            {
              aDistance += (m_RangeDecoder.DecodeDirectBits(kDistDirectBits[aPosSlot] - 
                  kNumAlignBits) << kNumAlignBits);
              aDistance += m_PosAlignDecoder.Decode(&m_RangeDecoder);
            }
          }
          else
            aDistance = aPosSlot;

          
          aRepDistances[3] = aRepDistances[2];
          aRepDistances[2] = aRepDistances[1];
          aRepDistances[1] = aRepDistances[0];
          
          aRepDistances[0] = aDistance;
          // UpdateStat(aLen, aPosSlot);
        }
        if (aDistance >= aNowPos64)
          throw "data error";
        m_OutWindowStream.CopyBackBlock(aDistance, aLen);
        aNowPos64 += aLen;
        aPreviousByte = m_OutWindowStream.GetOneByte(0 - 1);
      }
    }
    if (aProgress != NULL && aNowPos64 - aProgressPosValuePrev >= kStepSize)
    {
      UINT64 anInSize = m_RangeDecoder.GetProcessedSize();
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&anInSize, &aNowPos64));
      aProgressPosValuePrev = aNowPos64;
    }
  }
  return Flush();
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress)
{
  try
  {
    return CodeReal(anInStream, anOutStream, anInSize, anOutSize, aProgress);
  }
  catch(const NStream::NWindow::COutWriteException &anOutWriteException)
  {
    return anOutWriteException.m_Result;
  }
  catch(...)
  {
    return S_FALSE;
  }
}

STDMETHODIMP CDecoder::SetDecoderProperties(ISequentialInStream *anInStream)
{
  UINT32 aNumPosStateBits;
  UINT32 aLiteralPosStateBits;
  UINT32 aLiteralContextBits;
  UINT32 aDictionarySize;
  RETURN_IF_NOT_S_OK(DecodeProperties(anInStream, 
      aNumPosStateBits,
      aLiteralPosStateBits,
      aLiteralContextBits, 
      aDictionarySize));
  RETURN_IF_NOT_S_OK(SetDictionarySize(aDictionarySize));
  RETURN_IF_NOT_S_OK(SetLiteralProperties(aLiteralPosStateBits, aLiteralContextBits));
  RETURN_IF_NOT_S_OK(SetPosBitsProperties(aNumPosStateBits));
  return S_OK;
}

}}
