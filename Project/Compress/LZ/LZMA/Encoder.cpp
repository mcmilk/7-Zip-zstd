// LZMA/Encoder.cpp

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
    for (BYTE slotFast = 0; slotFast < kFastSlots; slotFast++)
    {
      UINT32 k = (1 << kDistDirectBits[slotFast]);
      for (UINT32 j = 0; j < k; j++, c++)
        g_FastPos[c] = slotFast;
    }
  }
} g_FastPosInit;

const kDefaultDictionaryLogSize = 20;
const kNumFastBytesDefault = 0x20;

CEncoder::CEncoder():
  _dictionarySize(1 << kDefaultDictionaryLogSize),
  _dictionarySizePrev(UINT32(-1)),
  _numFastBytes(kNumFastBytesDefault),
  _numFastBytesPrev(UINT32(-1)),
  _distTableSize(kDefaultDictionaryLogSize * 2),
  _posStateBits(2),
  _posStateMask(4 - 1),
  _numLiteralPosStateBits(0),
  _numLiteralContextBits(3)
{
  _maxMode = false;
  _fastMode = false;
  _posAlignEncoder.Create(kNumAlignBits);
  for(int i = 0; i < kNumPosModels; i++)
    _posEncoders[i].Create(kDistDirectBits[kStartPosModelIndex + i]);
}

HRESULT CEncoder::Create()
{
  if (_dictionarySize == _dictionarySizePrev && _numFastBytesPrev == _numFastBytes)
    return S_OK;
  RETURN_IF_NOT_S_OK(_matchFinder->Create(_dictionarySize, kNumOpts, _numFastBytes, 
      kMatchMaxLen - _numFastBytes));
  _dictionarySizePrev = _dictionarySize;
  _numFastBytesPrev = _numFastBytes;
  _literalEncoder.Create(_numLiteralPosStateBits, _numLiteralContextBits);
  _lenEncoder.Create(1 << _posStateBits);
  _repMatchLenEncoder.Create(1 << _posStateBits);
  return S_OK;
}


// ICompressSetEncoderProperties2
STDMETHODIMP CEncoder::SetEncoderProperties2(const PROPID *propIDs, 
    const PROPVARIANT *properties, UINT32 numProperties)
{
  for (UINT32 i = 0; i < numProperties; i++)
  {
    const PROPVARIANT &property = properties[i];
    switch(propIDs[i])
    {
      case NEncodingProperies::kNumFastBytes:
      {
        if (properties[i].vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 numFastBytes = properties[i].ulVal;
        if(numFastBytes < 2 || numFastBytes > kMatchMaxLen)
          return E_INVALIDARG;
        _numFastBytes = numFastBytes;
        break;
      }
      case NEncodingProperies::kAlgorithm:
      {
        if (properties[i].vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 maximize = properties[i].ulVal;
        _fastMode = (maximize == 0); 
        _maxMode = (maximize >= 2);
        break;
      }
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

// ICompressSetCoderProperties
STDMETHODIMP CEncoder::SetCoderProperties2(const PROPID *propIDs, 
    const PROPVARIANT *properties, UINT32 numProperties)
{
  for (UINT32 i = 0; i < numProperties; i++)
  {
    const PROPVARIANT &property = properties[i];
    switch(propIDs[i])
    {
      case NEncodedStreamProperies::kDictionarySize:
      {
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 dictionarySize = property.ulVal;
        if (dictionarySize < UINT32(1 << kDicLogSizeMin) ||
            dictionarySize > UINT32(1 << kDicLogSizeMax))
          return E_INVALIDARG;
        _dictionarySize = dictionarySize;
        UINT32 dicLogSize;
        for(dicLogSize = 0; dicLogSize < kDicLogSizeMax; dicLogSize++)
          if (dictionarySize <= (UINT32(1) << dicLogSize))
            break;
        _distTableSize = dicLogSize * 2;
        break;
      }
      case NEncodedStreamProperies::kPosStateBits:
      {
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 value = property.ulVal;
        if (value > NLength::kNumPosStatesBitsEncodingMax)
          return E_INVALIDARG;
        _posStateBits = value;
        _posStateMask = (1 << _posStateBits) - 1;
        break;
      }
      case NEncodedStreamProperies::kLitPosBits:
      {
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 value = property.ulVal;
        if (value > kNumLitPosStatesBitsEncodingMax)
          return E_INVALIDARG;
        _numLiteralPosStateBits = value;
        break;
      }
      case NEncodedStreamProperies::kLitContextBits:
      {
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        UINT32 value = property.ulVal;
        if (value > kNumLitContextBitsMax)
          return E_INVALIDARG;
        _numLiteralContextBits = value;
        break;
      }
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{ 
  BYTE firstByte = (_posStateBits * 5 + _numLiteralPosStateBits) * 9 + _numLiteralContextBits;
  RETURN_IF_NOT_S_OK(outStream->Write(&firstByte, sizeof(firstByte), NULL));
  return outStream->Write(&_dictionarySize, sizeof(_dictionarySize), NULL);
}


STDMETHODIMP CEncoder::Init(ISequentialInStream *inStream, 
    ISequentialOutStream *outStream)
{
  CBaseCoder::Init();

  RETURN_IF_NOT_S_OK(_matchFinder->Init(inStream));
  _rangeEncoder.Init(outStream);

  int i;
  for(i = 0; i < kNumStates; i++)
  {
    for (UINT32 j = 0; j <= _posStateMask; j++)
    {
      _mainChoiceEncoders[i][j].Init();
      _matchRepShortChoiceEncoders[i][j].Init();
    }
    _matchChoiceEncoders[i].Init();
    _matchRepChoiceEncoders[i].Init();
    _matchRep1ChoiceEncoders[i].Init();
    _matchRep2ChoiceEncoders[i].Init();
  }

  _literalEncoder.Init();

  // _repMatchLenEncoder.Init();
  
  for(i = 0; i < kNumLenToPosStates; i++)
    _posSlotEncoder[i].Init();

  for(i = 0; i < kNumPosModels; i++)
    _posEncoders[i].Init();

  _lenEncoder.Init();
  _repMatchLenEncoder.Init();

  _posAlignEncoder.Init();

  _longestMatchWasFound = false;
  _optimumEndIndex = 0;
  _optimumCurrentIndex = 0;
  _additionalOffset = 0;

  return S_OK;
}

void CEncoder::MovePos(UINT32 num)
{
  for (;num > 0; num--)
  {
    _matchFinder->DummyLongestMatch();
    HRESULT result = _matchFinder->MovePos();
    if (result != S_OK)
      throw CMatchFinderException(result);
    _additionalOffset++;
  }
}

UINT32 CEncoder::Backward(UINT32 &backRes, UINT32 cur)
{
  _optimumEndIndex = cur;
  UINT32 posMem = _optimum[cur].PosPrev;
  UINT32 backMem = _optimum[cur].BackPrev;
  do
  {
    if (_optimum[cur].Prev1IsChar)
    {
      _optimum[posMem].MakeAsChar();
      _optimum[posMem].PosPrev = posMem - 1;
      if (_optimum[cur].Prev2)
      {
        _optimum[posMem - 1].Prev1IsChar = false;
        _optimum[posMem - 1].PosPrev = _optimum[cur].PosPrev2;
        _optimum[posMem - 1].BackPrev = _optimum[cur].BackPrev2;
      }
    }
    UINT32 posPrev = posMem;
    UINT32 backCur = backMem;

    backMem = _optimum[posPrev].BackPrev;
    posMem = _optimum[posPrev].PosPrev;

    _optimum[posPrev].BackPrev = backCur;
    _optimum[posPrev].PosPrev = cur;
    cur = posPrev;
  }
  while(cur > 0);
  backRes = _optimum[0].BackPrev;
  _optimumCurrentIndex  = _optimum[0].PosPrev;
  return _optimumCurrentIndex; 
}

/*
inline UINT32 GetMatchLen(const BYTE *data, UINT32 back, UINT32 limit)
{  
  back++;
  for(UINT32 i = 0; i < limit && data[i] == data[i - back]; i++);
  return i;
}
*/

UINT32 CEncoder::GetOptimum(UINT32 &backRes, UINT32 position)
{
  if(_optimumEndIndex != _optimumCurrentIndex)
  {
    UINT32 len = _optimum[_optimumCurrentIndex].PosPrev - _optimumCurrentIndex;
    backRes = _optimum[_optimumCurrentIndex].BackPrev;
    _optimumCurrentIndex = _optimum[_optimumCurrentIndex].PosPrev;
    return len;
  }
  _optimumCurrentIndex = 0;
  _optimumEndIndex = 0; // test it;
  
  UINT32 lenMain;
  if (!_longestMatchWasFound)
    lenMain = ReadMatchDistances();
  else
  {
    lenMain = _longestMatchLength;
    _longestMatchWasFound = false;
  }


  UINT32 reps[kNumRepDistances];
  UINT32 repLens[kNumRepDistances];
  UINT32 repMaxIndex = 0;
  int i;
  for(i = 0; i < kNumRepDistances; i++)
  {
    reps[i] = _repDistances[i];
    repLens[i] = _matchFinder->GetMatchLen(0 - 1, reps[i], kMatchMaxLen);
    if (i == 0 || repLens[i] > repLens[repMaxIndex])
      repMaxIndex = i;
  }
  if(repLens[repMaxIndex] > _numFastBytes)
  {
    backRes = repMaxIndex;
    MovePos(repLens[repMaxIndex] - 1);
    return repLens[repMaxIndex];
  }

  if(lenMain > _numFastBytes)
  {
    UINT32 backMain = (lenMain < _numFastBytes) ? _matchDistances[lenMain] :
        _matchDistances[_numFastBytes];
    backRes = backMain + kNumRepDistances; 
    MovePos(lenMain - 1);
    return lenMain;
  }
  BYTE currentByte = _matchFinder->GetIndexByte(0 - 1);

  _optimum[0].State = _state;

  BYTE matchByte;
  
  matchByte = _matchFinder->GetIndexByte(0 - _repDistances[0] - 1 - 1);

  UINT32 posState = (position & _posStateMask);

  _optimum[1].Price = _mainChoiceEncoders[_state.Index][posState].GetPrice(kMainChoiceLiteralIndex) + 
      _literalEncoder.GetPrice(position, _previousByte, _peviousIsMatch, matchByte, currentByte);
  _optimum[1].MakeAsChar();

  _optimum[1].PosPrev = 0;

  for (i = 0; i < kNumRepDistances; i++)
    _optimum[0].Backs[i] = reps[i];

  UINT32 matchPrice = _mainChoiceEncoders[_state.Index][posState].GetPrice(kMainChoiceMatchIndex);
  UINT32 repMatchPrice = matchPrice + 
      _matchChoiceEncoders[_state.Index].GetPrice(kMatchChoiceRepetitionIndex);

  if(matchByte == currentByte)
  {
    UINT32 shortRepPrice = repMatchPrice + GetRepLen1Price(_state, posState);
    if(shortRepPrice < _optimum[1].Price)
    {
      _optimum[1].Price = shortRepPrice;
      _optimum[1].MakeAsShortRep();
    }
  }
  if(lenMain < 2)
  {
    backRes = _optimum[1].BackPrev;
    return 1;
  }

  
  UINT32 normalMatchPrice = matchPrice + 
      _matchChoiceEncoders[_state.Index].GetPrice(kMatchChoiceDistanceIndex);

  if (lenMain <= repLens[repMaxIndex])
    lenMain = 0;

  UINT32 len;
  for(len = 2; len <= lenMain; len++)
  {
    _optimum[len].PosPrev = 0;
    _optimum[len].BackPrev = _matchDistances[len] + kNumRepDistances;
    _optimum[len].Price = normalMatchPrice + 
        GetPosLenPrice(_matchDistances[len], len, posState);
    _optimum[len].Prev1IsChar = false;
  }

  if (lenMain < repLens[repMaxIndex])
    lenMain = repLens[repMaxIndex];

  for (; len <= lenMain; len++)
    _optimum[len].Price = kIfinityPrice;

  for(i = 0; i < kNumRepDistances; i++)
  {
    UINT repLen = repLens[i];
    for(UINT32 lenTest = 2; lenTest <= repLen; lenTest++)
    {
      UINT32 curAndLenPrice = repMatchPrice + GetRepPrice(i, lenTest, _state, posState);
      COptimal &optimum = _optimum[lenTest];
      if (curAndLenPrice < optimum.Price) 
      {
        optimum.Price = curAndLenPrice;
        optimum.PosPrev = 0;
        optimum.BackPrev = i;
        optimum.Prev1IsChar = false;
      }
    }
  }

  UINT32 cur = 0;
  UINT32 lenEnd = lenMain;

  while(true)
  {
    cur++;
    if(cur == lenEnd)  
      return Backward(backRes, cur);
    position++;
    UINT32 posPrev = _optimum[cur].PosPrev;
    CState state;
    if (_optimum[cur].Prev1IsChar)
    {
      posPrev--;
      if (_optimum[cur].Prev2)
      {
        state = _optimum[_optimum[cur].PosPrev2].State;
        if (_optimum[cur].BackPrev2 < kNumRepDistances)
          state.UpdateRep();
        else
          state.UpdateMatch();
      }
      else
        state = _optimum[posPrev].State;
      state.UpdateChar();
    }
    else
      state = _optimum[posPrev].State;
    bool prevWasMatch;
    if (posPrev == cur - 1)
    {
      if (_optimum[cur].IsShortRep())
      {
        prevWasMatch = true;
        state.UpdateShortRep();
      }
      else
      {
        prevWasMatch = false;
        state.UpdateChar();
      }
      /*
      if (_optimum[cur].Prev1IsChar)
        for(int i = 0; i < kNumRepDistances; i++)
          reps[i] = _optimum[posPrev].Backs[i];
      */
    }
    else
    {
      prevWasMatch = true;
      UINT32 pos;
      if (_optimum[cur].Prev1IsChar && _optimum[cur].Prev2)
      {
        posPrev = _optimum[cur].PosPrev2;
        pos = _optimum[cur].BackPrev2;
        state.UpdateRep();
      }
      else
      {
        pos = _optimum[cur].BackPrev;
        if (pos < kNumRepDistances)
          state.UpdateRep();
        else
          state.UpdateMatch();
      }
      if (pos < kNumRepDistances)
      {
        reps[0] = _optimum[posPrev].Backs[pos];
    		UINT32 i;
        for(i = 1; i <= pos; i++)
          reps[i] = _optimum[posPrev].Backs[i - 1];
        for(; i < kNumRepDistances; i++)
          reps[i] = _optimum[posPrev].Backs[i];
      }
      else
      {
        reps[0] = (pos - kNumRepDistances);
        for(UINT32 i = 1; i < kNumRepDistances; i++)
          reps[i] = _optimum[posPrev].Backs[i - 1];
      }
    }
    _optimum[cur].State = state;
    for(UINT32 i = 0; i < kNumRepDistances; i++)
      _optimum[cur].Backs[i] = reps[i];
    UINT32 newLen = ReadMatchDistances();
    if(newLen > _numFastBytes)
    {
      _longestMatchLength = newLen;
      _longestMatchWasFound = true;
      return Backward(backRes, cur);
    }
    UINT32 curPrice = _optimum[cur].Price; 
    // BYTE currentByte  = _matchFinder->GetIndexByte(0 - 1);
    // BYTE matchByte = _matchFinder->GetIndexByte(0 - reps[0] - 1 - 1);
    const BYTE *data = _matchFinder->GetPointerToCurrentPos() - 1;
    BYTE currentByte = *data;
    BYTE matchByte = data[0 - reps[0] - 1];

    UINT32 posState = (position & _posStateMask);

    UINT32 curAnd1Price = curPrice +
        _mainChoiceEncoders[state.Index][posState].GetPrice(kMainChoiceLiteralIndex) +
        _literalEncoder.GetPrice(position, data[-1], prevWasMatch, matchByte, currentByte);

    COptimal &nextOptimum = _optimum[cur + 1];

    bool nextIsChar = false;
    if (curAnd1Price < nextOptimum.Price) 
    {
      nextOptimum.Price = curAnd1Price;
      nextOptimum.PosPrev = cur;
      nextOptimum.MakeAsChar();
      nextIsChar = true;
    }

    UINT32 matchPrice = curPrice + _mainChoiceEncoders[state.Index][posState].GetPrice(kMainChoiceMatchIndex);
    UINT32 repMatchPrice = matchPrice + _matchChoiceEncoders[state.Index].GetPrice(kMatchChoiceRepetitionIndex);
    
    if(matchByte == currentByte &&
        !(nextOptimum.PosPrev < cur && nextOptimum.BackPrev == 0))
    {
      UINT32 shortRepPrice = repMatchPrice + GetRepLen1Price(state, posState);
      if(shortRepPrice <= nextOptimum.Price)
      {
        nextOptimum.Price = shortRepPrice;
        nextOptimum.PosPrev = cur;
        nextOptimum.MakeAsShortRep();
        // nextIsChar = false;
      }
    }
    /*
    if(newLen == 2 && _matchDistances[2] >= kDistLimit2) // test it maybe set 2000 ?
      continue;
    */

    UINT32 numAvailableBytes = _matchFinder->GetNumAvailableBytes() + 1;
    numAvailableBytes = MyMin(kNumOpts - 1 - cur, numAvailableBytes);

    if (numAvailableBytes < 2)
      continue;
    if (numAvailableBytes > _numFastBytes)
      numAvailableBytes = _numFastBytes;
    if (numAvailableBytes >= 3 && !nextIsChar)
    {
      UINT32 backOffset = reps[0] + 1;
      UINT32 temp;
      for (temp = 1; temp < numAvailableBytes; temp++)
        if (data[temp] != data[temp - backOffset])
          break;
      UINT32 lenTest2 = temp - 1;
      if (lenTest2 >= 2)
      {
        CState state2 = state;
        state2.UpdateChar();
        UINT32 posStateNext = (position + 1) & _posStateMask;
        UINT32 nextRepMatchPrice = curAnd1Price + 
            _mainChoiceEncoders[state2.Index][posStateNext].GetPrice(kMainChoiceMatchIndex) +
            _matchChoiceEncoders[state2.Index].GetPrice(kMatchChoiceRepetitionIndex);
        // for (; lenTest2 >= 2; lenTest2--)
        {
          while(lenEnd < cur + 1 + lenTest2)
            _optimum[++lenEnd].Price = kIfinityPrice;
          UINT32 curAndLenPrice = nextRepMatchPrice + GetRepPrice(
              0, lenTest2, state2, posStateNext);
          COptimal &optimum = _optimum[cur + 1 + lenTest2];
          if (curAndLenPrice < optimum.Price) 
          {
            optimum.Price = curAndLenPrice;
            optimum.PosPrev = cur + 1;
            optimum.BackPrev = 0;
            optimum.Prev1IsChar = true;
            optimum.Prev2 = false;
          }
        }
      }
    }
    for(UINT32 repIndex = 0; repIndex < kNumRepDistances; repIndex++)
    {
      // UINT32 repLen = _matchFinder->GetMatchLen(0 - 1, reps[repIndex], newLen); // test it;
      UINT32 backOffset = reps[repIndex] + 1;
      UINT32 lenTest;
      for (lenTest = 0; lenTest < numAvailableBytes; lenTest++)
        if (data[lenTest] != data[lenTest - backOffset])
          break;
      for(; lenTest >= 2; lenTest--)
      {
        while(lenEnd < cur + lenTest)
          _optimum[++lenEnd].Price = kIfinityPrice;
        UINT32 curAndLenPrice = repMatchPrice + GetRepPrice(repIndex, lenTest, state, posState);
        COptimal &optimum = _optimum[cur + lenTest];
        if (curAndLenPrice < optimum.Price) 
        {
          optimum.Price = curAndLenPrice;
          optimum.PosPrev = cur;
          optimum.BackPrev = repIndex;
          optimum.Prev1IsChar = false;
        }

        /*
        if (_maxMode)
        {
          UINT32 temp;
          for (temp = lenTest + 1; temp < numAvailableBytes; temp++)
            if (data[temp] != data[temp - backOffset])
              break;
          UINT32 lenTest2 = temp - (lenTest + 1);
          if (lenTest2 >= 2)
          {
            CState state2 = state;
            state2.UpdateRep();
            UINT32 posStateNext = (position + lenTest) & _posStateMask;
            UINT32 curAndLenCharPrice = curAndLenPrice + 
                _mainChoiceEncoders[state2.Index][posStateNext].GetPrice(kMainChoiceLiteralIndex) +
                _literalEncoder.GetPrice(position + lenTest, data[lenTest - 1], 
                true, data[lenTest - backOffset], data[lenTest]);
            state2.UpdateChar();
            posStateNext = (position + lenTest + 1) & _posStateMask;
            UINT32 nextMatchPrice = curAndLenCharPrice + _mainChoiceEncoders[state2.Index][posStateNext].GetPrice(kMainChoiceMatchIndex);
            UINT32 nextRepMatchPrice = nextMatchPrice + _matchChoiceEncoders[state2.Index].GetPrice(kMatchChoiceRepetitionIndex);
            
            // for(; lenTest2 >= 2; lenTest2--)
            {
              UINT32 offset = lenTest + 1 + lenTest2;
              while(lenEnd < cur + offset)
                _optimum[++lenEnd].Price = kIfinityPrice;
              UINT32 curAndLenPrice = nextRepMatchPrice + GetRepPrice(
                  0, lenTest2, state2, posStateNext);
              COptimal &optimum = _optimum[cur + offset];
              if (curAndLenPrice < optimum.Price) 
              {
                optimum.Price = curAndLenPrice;
                optimum.PosPrev = cur + lenTest + 1;
                optimum.BackPrev = 0;
                optimum.Prev1IsChar = true;
                optimum.Prev2 = true;
                optimum.PosPrev2 = cur;
                optimum.BackPrev2 = repIndex;
              }
            }
          }
        }
        */
      }
    }
    
    //    for(UINT32 lenTest = 2; lenTest <= newLen; lenTest++)
    if (newLen > numAvailableBytes)
      newLen = numAvailableBytes;
    if (newLen >= 2)
    {
      if (newLen == 2 && _matchDistances[2] >= 0x80)
        continue;
      UINT32 normalMatchPrice = matchPrice + 
        _matchChoiceEncoders[state.Index].GetPrice(kMatchChoiceDistanceIndex);
      while(lenEnd < cur + newLen)
        _optimum[++lenEnd].Price = kIfinityPrice;

      for(UINT32 lenTest = newLen; lenTest >= 2; lenTest--)
      {
        UINT32 curBack = _matchDistances[lenTest];
        UINT32 curAndLenPrice = normalMatchPrice + GetPosLenPrice(curBack, lenTest, posState);
        COptimal &optimum = _optimum[cur + lenTest];
        if (curAndLenPrice < optimum.Price) 
        {
          optimum.Price = curAndLenPrice;
          optimum.PosPrev = cur;
          optimum.BackPrev = curBack + kNumRepDistances;
          optimum.Prev1IsChar = false;
        }

        if (_maxMode)
        {
          UINT32 backOffset = curBack + 1;
          UINT32 temp;
          for (temp = lenTest + 1; temp < numAvailableBytes; temp++)
            if (data[temp] != data[temp - backOffset])
              break;
          UINT32 lenTest2 = temp - (lenTest + 1);
          if (lenTest2 >= 2)
          {
            CState state2 = state;
            state2.UpdateMatch();
            UINT32 posStateNext = (position + lenTest) & _posStateMask;
            UINT32 curAndLenCharPrice = curAndLenPrice + 
                _mainChoiceEncoders[state2.Index][posStateNext].GetPrice(kMainChoiceLiteralIndex) +
                _literalEncoder.GetPrice(position + lenTest, data[lenTest - 1], 
                true, data[lenTest - backOffset], data[lenTest]);
            state2.UpdateChar();
            posStateNext = (position + lenTest + 1) & _posStateMask;
            UINT32 nextMatchPrice = curAndLenCharPrice + _mainChoiceEncoders[state2.Index][posStateNext].GetPrice(kMainChoiceMatchIndex);
            UINT32 nextRepMatchPrice = nextMatchPrice + _matchChoiceEncoders[state2.Index].GetPrice(kMatchChoiceRepetitionIndex);
            
            // for(; lenTest2 >= 2; lenTest2--)
            {
              UINT32 offset = lenTest + 1 + lenTest2;
              while(lenEnd < cur + offset)
                _optimum[++lenEnd].Price = kIfinityPrice;
              UINT32 curAndLenPrice = nextRepMatchPrice + GetRepPrice(
                  0, lenTest2, state2, posStateNext);
              COptimal &optimum = _optimum[cur + offset];
              if (curAndLenPrice < optimum.Price) 
              {
                optimum.Price = curAndLenPrice;
                optimum.PosPrev = cur + lenTest + 1;
                optimum.BackPrev = 0;
                optimum.Prev1IsChar = true;
                optimum.Prev2 = true;
                optimum.PosPrev2 = cur;
                optimum.BackPrev2 = curBack + kNumRepDistances;
              }
            }
          }
        }
      }
    }
  }
}

static bool inline ChangePair(UINT32 smallDist, UINT32 bigDist)
{
  const kDif = 7;
  return (smallDist < (UINT32(1) << (32-kDif)) && bigDist >= (smallDist << kDif));
}


UINT32 CEncoder::GetOptimumFast(UINT32 &backRes, UINT32 position)
{
  UINT32 lenMain;
  if (!_longestMatchWasFound)
    lenMain = ReadMatchDistances();
  else
  {
    lenMain = _longestMatchLength;
    _longestMatchWasFound = false;
  }
  UINT32 repLens[kNumRepDistances];
  UINT32 repMaxIndex = 0;
  for(int i = 0; i < kNumRepDistances; i++)
  {
    repLens[i] = _matchFinder->GetMatchLen(0 - 1, _repDistances[i], kMatchMaxLen);
    if (i == 0 || repLens[i] > repLens[repMaxIndex])
      repMaxIndex = i;
  }
  if(repLens[repMaxIndex] >= _numFastBytes)
  {
    backRes = repMaxIndex;
    MovePos(repLens[repMaxIndex] - 1);
    return repLens[repMaxIndex];
  }
  if(lenMain >= _numFastBytes)
  {
    backRes = _matchDistances[_numFastBytes] + kNumRepDistances; 
    MovePos(lenMain - 1);
    return lenMain;
  }
  while (lenMain > 2)
  {
    if (!ChangePair(_matchDistances[lenMain - 1], _matchDistances[lenMain]))
      break;
    lenMain--;
  }
  if (lenMain == 2 && _matchDistances[2] >= 0x80)
    lenMain = 1;

  UINT32 backMain = _matchDistances[lenMain];
  if (repLens[repMaxIndex] >= 2)
  {
    if (repLens[repMaxIndex] + 1 >= lenMain || 
        repLens[repMaxIndex] + 2 >= lenMain && (backMain > (1<<12)))
    {
      backRes = repMaxIndex;
      MovePos(repLens[repMaxIndex] - 1);
      return repLens[repMaxIndex];
    }
  }
  

  if (lenMain >= 2)
  {
    _longestMatchLength = ReadMatchDistances();
    if (_longestMatchLength >= 2 &&
      (
        (_longestMatchLength >= lenMain && 
          _matchDistances[lenMain] < backMain) || 
        _longestMatchLength == lenMain + 1 && 
          !ChangePair(backMain, _matchDistances[_longestMatchLength]) ||
        _longestMatchLength > lenMain + 1 ||
        _longestMatchLength + 1 >= lenMain && 
          ChangePair(_matchDistances[lenMain - 1], backMain)
      )
      )
    {
      _longestMatchWasFound = true;
      backRes = UINT32(-1);
      return 1;
    }
    for(int i = 0; i < kNumRepDistances; i++)
    {
      UINT32 repLen = _matchFinder->GetMatchLen(0 - 1, _repDistances[i], kMatchMaxLen);
      if (repLen >= 2 && repLen + 1 >= lenMain)
      {
        _longestMatchWasFound = true;
        backRes = UINT32(-1);
        return 1;
      }
    }
    backRes = backMain + kNumRepDistances; 
    MovePos(lenMain - 2);
    return lenMain;
  }
  backRes = UINT32(-1);
  return 1;
}

STDMETHODIMP CEncoder::InitMatchFinder(IInWindowStreamMatch *matchFinder)
{
  _matchFinder = matchFinder;
  return S_OK;
}

HRESULT CEncoder::Flush()
{
  _rangeEncoder.FlushData();
  return _rangeEncoder.FlushStream();
}

HRESULT CEncoder::CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress)
{
  RETURN_IF_NOT_S_OK(Create());
  Init(inStream, outStream);
  CCoderReleaser releaser(this);

  if (_matchFinder->GetNumAvailableBytes() == 0)
    return Flush();

  if (!_fastMode)
  {
    FillPosSlotPrices();
    FillDistancesPrices();
    FillAlignPrices();
  }

  _lenEncoder.SetTableSize(_numFastBytes);
  _lenEncoder.UpdateTables();
  _repMatchLenEncoder.SetTableSize(_numFastBytes);
  _repMatchLenEncoder.UpdateTables();

  UINT64 lastPosSlotFillingPos = 0;

  UINT64 progressPosValuePrev = 0;
  UINT64 nowPos64 = 0;
  ReadMatchDistances();
  UINT32 posState = UINT32(nowPos64) & _posStateMask;
  _mainChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, kMainChoiceLiteralIndex);
  _state.UpdateChar();
  BYTE curByte = _matchFinder->GetIndexByte(0 - _additionalOffset);
  _literalEncoder.Encode(&_rangeEncoder, UINT32(nowPos64), _previousByte, 
      false, 0, curByte);
  _previousByte = curByte;
  _additionalOffset--;
  nowPos64++;
  if (_matchFinder->GetNumAvailableBytes() == 0)
    return Flush();
  while(true)
  {
    UINT32 pos;
    UINT32 posState = UINT32(nowPos64) & _posStateMask;

    UINT32 len;
    if (_fastMode)
      len = GetOptimumFast(pos, UINT32(nowPos64));
    else
      len = GetOptimum(pos, UINT32(nowPos64));

    if(len == 1 && pos == (-1))
    {
      _mainChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, kMainChoiceLiteralIndex);
      _state.UpdateChar();
      BYTE matchByte;
      if(_peviousIsMatch)
        matchByte = _matchFinder->GetIndexByte(0 - _repDistances[0] - 1 - _additionalOffset);
      BYTE curByte = _matchFinder->GetIndexByte(0 - _additionalOffset);
      _literalEncoder.Encode(&_rangeEncoder, UINT32(nowPos64), _previousByte, _peviousIsMatch, 
          matchByte, curByte);
      _previousByte = curByte;
      _peviousIsMatch = false;
    }
    else
    {
      _peviousIsMatch = true;
      _mainChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, kMainChoiceMatchIndex);
      if(pos < kNumRepDistances)
      {
        _matchChoiceEncoders[_state.Index].Encode(&_rangeEncoder, kMatchChoiceRepetitionIndex);
        if(pos == 0)
        {
          _matchRepChoiceEncoders[_state.Index].Encode(&_rangeEncoder, 0);
          if(len == 1)
            _matchRepShortChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, 0);
          else
            _matchRepShortChoiceEncoders[_state.Index][posState].Encode(&_rangeEncoder, 1);
        }
        else
        {
          _matchRepChoiceEncoders[_state.Index].Encode(&_rangeEncoder, 1);
          if (pos == 1)
            _matchRep1ChoiceEncoders[_state.Index].Encode(&_rangeEncoder, 0);
          else
          {
            _matchRep1ChoiceEncoders[_state.Index].Encode(&_rangeEncoder, 1);
            _matchRep2ChoiceEncoders[_state.Index].Encode(&_rangeEncoder, pos - 2);
          }
        }
        if (len == 1)
          _state.UpdateShortRep();
        else
        {
          _repMatchLenEncoder.Encode(&_rangeEncoder, len - kMatchMinLen, posState);
          _state.UpdateRep();
        }


        UINT32 distance = _repDistances[pos];
        if (pos != 0)
        {
          for(UINT32 i = pos; i >= 1; i--)
            _repDistances[i] = _repDistances[i - 1];
          _repDistances[0] = distance;
        }
      }
      else
      {
        _matchChoiceEncoders[_state.Index].Encode(&_rangeEncoder, kMatchChoiceDistanceIndex);
        _state.UpdateMatch();
        _lenEncoder.Encode(&_rangeEncoder, len - kMatchMinLen, posState);
        pos -= kNumRepDistances;
        UINT32 posSlot = GetPosSlot(pos);
        UINT32 lenToPosState = GetLenToPosState(len);
        _posSlotEncoder[lenToPosState].Encode(&_rangeEncoder, posSlot);
        
        UINT32 footerBits = kDistDirectBits[posSlot];
        UINT32 posReduced = pos - kDistStart[posSlot];
        if (posSlot >= kStartPosModelIndex)
        {
          if (posSlot < kEndPosModelIndex)
            _posEncoders[posSlot - kStartPosModelIndex].Encode(&_rangeEncoder, posReduced);
          else
          {
            _rangeEncoder.EncodeDirectBits(posReduced >> kNumAlignBits, footerBits - kNumAlignBits);
            _posAlignEncoder.Encode(&_rangeEncoder, posReduced & kAlignMask);
            if (!_fastMode)
              if (--_alignPriceCount == 0)
                FillAlignPrices();
          }
        }
        UINT32 distance = pos;
        for(UINT32 i = kNumRepDistances - 1; i >= 1; i--)
          _repDistances[i] = _repDistances[i - 1];
        _repDistances[0] = distance;
      }
      _previousByte = _matchFinder->GetIndexByte(len - 1 - _additionalOffset);
    }
    _additionalOffset -= len;
    nowPos64 += len;
    if (!_fastMode)
      if (nowPos64 - lastPosSlotFillingPos >= (1 << 9))
      {
        FillPosSlotPrices();
        FillDistancesPrices();
        lastPosSlotFillingPos = nowPos64;
      }
    if (nowPos64 - progressPosValuePrev >= (1 << 12) && progress != NULL)
    {
      UINT64 outSize = _rangeEncoder.GetProcessedSize();
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&nowPos64, &outSize));
      progressPosValuePrev = nowPos64;
    }
    if (_additionalOffset == 0 && _matchFinder->GetNumAvailableBytes() == 0)
      return Flush();
  }
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  try
  {
    return CodeReal(inStream, outStream, inSize, outSize, progress);
  }
  catch(CMatchFinderException &exception)
  {
    return exception.Result;
  }
  catch(const NStream::COutByteWriteException &exception)
  {
    return exception.Result;
  }
  catch(...)
  {
    return E_FAIL;
  }
}
  
void CEncoder::FillPosSlotPrices()
{
  for (int lenToPosState = 0; lenToPosState < kNumLenToPosStates; lenToPosState++)
  {
	  UINT32 posSlot;
    for (posSlot = 0; posSlot < kEndPosModelIndex && posSlot < _distTableSize; posSlot++)
      _posSlotPrices[lenToPosState][posSlot] = _posSlotEncoder[lenToPosState].GetPrice(posSlot);
    for (; posSlot < _distTableSize; posSlot++)
      _posSlotPrices[lenToPosState][posSlot] = _posSlotEncoder[lenToPosState].GetPrice(posSlot) + 
          ((kDistDirectBits[posSlot] - kNumAlignBits) << kNumBitPriceShiftBits);
  }
}

void CEncoder::FillDistancesPrices()
{
  for (int lenToPosState = 0; lenToPosState < kNumLenToPosStates; lenToPosState++)
  {
	  UINT32 i;
    for (i = 0; i < kStartPosModelIndex; i++)
      _distancesPrices[lenToPosState][i] = _posSlotPrices[lenToPosState][i];
    for (; i < kNumFullDistances; i++)
    { 
      UINT32 posSlot = GetPosSlot(i);
      _distancesPrices[lenToPosState][i] = _posSlotPrices[lenToPosState][posSlot] +
          _posEncoders[posSlot - kStartPosModelIndex].GetPrice(i - kDistStart[posSlot]);
    }
  }
}

void CEncoder::FillAlignPrices()
{
  for (int i = 0; i < kAlignTableSize; i++)
    _alignPrices[i] = _posAlignEncoder.GetPrice(i);
  _alignPriceCount = kAlignTableSize;
}

}}
