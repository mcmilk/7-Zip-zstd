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
    UINT32 sums[kLenTableMax];
    for (int len = 2; len < kLenTableMax; len++)
    {
      sums[len] = 0;
      for (int dist = 0; dist < kNumDists; dist++)
        sums[len] += g_Counts[len][dist];
      if (sums[len] == 0)
        sums[len] = 1;
    }
    for (int dist = 0; dist < kNumDists; dist++)
    {
      ofs << setw(4) << dist << "  ";
      for (int len = 2; len < kLenTableMax; len++)
      {
        ofs << setw(4) << g_Counts[len][dist] * 1000 / sums[len];
      }
      ofs << endl;
    }
  }
} g_Class;

void UpdateStat(UINT32 len, UINT32 dist)
{
  if (len >= kLenTableMax)
    len = kLenTableMax - 1;
  g_Counts[len][dist / 2]++;
}
*/

#define RETURN_E_OUTOFMEMORY_IF_FALSE(x) { if (!(x)) return E_OUTOFMEMORY; }

namespace NCompress {
namespace NLZMA {

HRESULT CDecoder::SetDictionarySize(UINT32 dictionarySize)
{
  if (_dictionarySize != dictionarySize)
  {
    _dictionarySize = dictionarySize;
    _dictionarySizeCheck = MyMax(_dictionarySize, UINT32(1));
    UINT32 blockSize = MyMax(_dictionarySizeCheck, UINT32(1 << 12));
    try
    {
      _outWindowStream.Create(blockSize /*, kMatchMaxLen */);
    }
    catch(...)
    {
      return E_OUTOFMEMORY;
    }
  }
  return S_OK;
}

HRESULT CDecoder::SetLiteralProperties(
    UINT32 numLiteralPosStateBits, UINT32 numLiteralContextBits)
{
  if (numLiteralPosStateBits > 8)
    return E_INVALIDARG;
  if (numLiteralContextBits > 8)
    return E_INVALIDARG;
  _literalDecoder.Create(numLiteralPosStateBits, numLiteralContextBits);
  return S_OK;
}

HRESULT CDecoder::SetPosBitsProperties(UINT32 numPosStateBits)
{
  if (numPosStateBits > NLength::kNumPosStatesBitsMax)
    return E_INVALIDARG;
  UINT32 numPosStates = 1 << numPosStateBits;
  _lenDecoder.Create(numPosStates);
  _repMatchLenDecoder.Create(numPosStates);
  _posStateMask = numPosStates - 1;
  return S_OK;
}

CDecoder::CDecoder():
  _dictionarySize(-1)
{
  Create();
}

HRESULT CDecoder::Create()
{
  for(int i = 0; i < kNumPosModels; i++)
  {
    RETURN_E_OUTOFMEMORY_IF_FALSE(
        _posDecoders[i].Create(kDistDirectBits[kStartPosModelIndex + i]));
  }
  return S_OK;
}


HRESULT CDecoder::Init(ISequentialInStream *inStream,
    ISequentialOutStream *outStream)
{
  _rangeDecoder.Init(inStream);

  _outWindowStream.Init(outStream);

  int i;
  for(i = 0; i < kNumStates; i++)
  {
    for (UINT32 j = 0; j <= _posStateMask; j++)
    {
      _mainChoiceDecoders[i][j].Init();
      _matchRepShortChoiceDecoders[i][j].Init();
    }
    _matchChoiceDecoders[i].Init();
    _matchRepChoiceDecoders[i].Init();
    _matchRep1ChoiceDecoders[i].Init();
    _matchRep2ChoiceDecoders[i].Init();
  }
  
  _literalDecoder.Init();
   
  // _repMatchLenDecoder.Init();

  for (i = 0; i < kNumLenToPosStates; i++)
    _posSlotDecoder[i].Init();

  for(i = 0; i < kNumPosModels; i++)
    _posDecoders[i].Init();
  
  _lenDecoder.Init();
  _repMatchLenDecoder.Init();

  _posAlignDecoder.Init();
  return S_OK;

}



STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, 
    const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  /*
  if (outSize == NULL)
    return E_INVALIDARG;
  */

  Init(inStream, outStream);
  CDecoderFlusher flusher(this);

  CState state;
  state.Init();
  bool peviousIsMatch = false;
  BYTE previousByte = 0;
  UINT32 repDistances[kNumRepDistances];
  for(int i = 0 ; i < kNumRepDistances; i++)
    repDistances[i] = 0;

  UINT64 nowPos64 = 0;
  UINT64 size = (outSize == NULL) ? (UINT64)(INT64)(-1) : *outSize;
  while(nowPos64 < size)
  {
    UINT64 nextPos = MyMin(nowPos64 + (1 << 18), size);
    while(nowPos64 < nextPos)
    {
      UINT32 posState = UINT32(nowPos64) & _posStateMask;
      if (_mainChoiceDecoders[state.Index][posState].Decode(&_rangeDecoder) == kMainChoiceLiteralIndex)
      {
        state.UpdateChar();
        if(peviousIsMatch)
        {
          BYTE matchByte = _outWindowStream.GetOneByte(0 - repDistances[0] - 1);
          previousByte = _literalDecoder.DecodeWithMatchByte(&_rangeDecoder, 
              UINT32(nowPos64), previousByte, matchByte);
          peviousIsMatch = false;
        }
        else
          previousByte = _literalDecoder.DecodeNormal(&_rangeDecoder, 
              UINT32(nowPos64), previousByte);
        _outWindowStream.PutOneByte(previousByte);
        nowPos64++;
      }
      else             
      {
        peviousIsMatch = true;
        UINT32 distance, len;
        if(_matchChoiceDecoders[state.Index].Decode(&_rangeDecoder) == 
            kMatchChoiceRepetitionIndex)
        {
          if(_matchRepChoiceDecoders[state.Index].Decode(&_rangeDecoder) == 0)
          {
            if(_matchRepShortChoiceDecoders[state.Index][posState].Decode(&_rangeDecoder) == 0)
            {
              state.UpdateShortRep();
              previousByte = _outWindowStream.GetOneByte(0 - repDistances[0] - 1);
              _outWindowStream.PutOneByte(previousByte);
              nowPos64++;
              continue;
            }
            distance = repDistances[0];
          }
          else
          {
            if(_matchRep1ChoiceDecoders[state.Index].Decode(&_rangeDecoder) == 0)
            {
              distance = repDistances[1];
              repDistances[1] = repDistances[0];
            }
            else 
            {
              if (_matchRep2ChoiceDecoders[state.Index].Decode(&_rangeDecoder) == 0)
              {
                distance = repDistances[2];
              }
              else
              {
                distance = repDistances[3];
                repDistances[3] = repDistances[2];
              }
              repDistances[2] = repDistances[1];
              repDistances[1] = repDistances[0];
            }
            repDistances[0] = distance;
          }
          len = _repMatchLenDecoder.Decode(&_rangeDecoder, posState) + kMatchMinLen;
          state.UpdateRep();
        }
        else
        {
          len = kMatchMinLen + _lenDecoder.Decode(&_rangeDecoder, posState);
          state.UpdateMatch();
          UINT32 posSlot = _posSlotDecoder[GetLenToPosState(len)].Decode(&_rangeDecoder);
          if (posSlot >= kStartPosModelIndex)
          {
            distance = kDistStart[posSlot];
            if (posSlot < kEndPosModelIndex)
              distance += _posDecoders[posSlot - kStartPosModelIndex].Decode(&_rangeDecoder);
            else
            {
              distance += (_rangeDecoder.DecodeDirectBits(kDistDirectBits[posSlot] - 
                  kNumAlignBits) << kNumAlignBits);
              distance += _posAlignDecoder.Decode(&_rangeDecoder);
            }
          }
          else
            distance = posSlot;

          
          repDistances[3] = repDistances[2];
          repDistances[2] = repDistances[1];
          repDistances[1] = repDistances[0];
          
          repDistances[0] = distance;
          // UpdateStat(len, posSlot);
        }
        if (distance >= nowPos64 || distance >= _dictionarySizeCheck)
        {
          if (distance == (UINT32)(-1) && size == (UINT64)(INT64)(-1))
          {
            flusher.NeedFlush = false;
            return Flush();
          }
          throw "data error";
        }
        _outWindowStream.CopyBackBlock(distance, len);
        nowPos64 += len;
        previousByte = _outWindowStream.GetOneByte(0 - 1);
      }
    }
    if (progress != NULL)
    {
      UINT64 inSize = _rangeDecoder.GetProcessedSize();
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&inSize, &nowPos64));
    }
  }
  flusher.NeedFlush = false;
  return Flush();
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress)
{
  try
  {
    return CodeReal(inStream, outStream, inSize, outSize, progress);
  }
  catch(const NStream::CInByteReadException &exception)
  {
    return exception.Result;
  }
  catch(const NStream::NWindow::COutWriteException &outWriteException)
  {
    return outWriteException.Result;
  }
  catch(...)
  {
    return S_FALSE;
  }
}

STDMETHODIMP CDecoder::SetDecoderProperties(ISequentialInStream *inStream)
{
  UINT32 numPosStateBits;
  UINT32 numLiteralPosStateBits;
  UINT32 numLiteralContextBits;
  UINT32 dictionarySize;
  RETURN_IF_NOT_S_OK(DecodeProperties(inStream, 
      numPosStateBits,
      numLiteralPosStateBits,
      numLiteralContextBits, 
      dictionarySize));
  RETURN_IF_NOT_S_OK(SetDictionarySize(dictionarySize));
  RETURN_IF_NOT_S_OK(SetLiteralProperties(numLiteralPosStateBits, numLiteralContextBits));
  RETURN_IF_NOT_S_OK(SetPosBitsProperties(numPosStateBits));
  return S_OK;
}

}}
