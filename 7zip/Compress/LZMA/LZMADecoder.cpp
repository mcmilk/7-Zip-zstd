// LZMADecoder.cpp

#include "StdAfx.h"

#include "LZMADecoder.h"
#include "../../../Common/Defs.h"

namespace NCompress {
namespace NLZMA {

void CDecoder::Init()
{
  { 
    for(int i = 0; i < kNumStates; i++)
    {
      for (UInt32 j = 0; j <= _posStateMask; j++)
      {
        _isMatch[i][j].Init();
        _isRep0Long[i][j].Init();
      }
      _isRep[i].Init();
      _isRepG0[i].Init();
      _isRepG1[i].Init();
      _isRepG2[i].Init();
    }
  }
  { 
    for (UInt32 i = 0; i < kNumLenToPosStates; i++)
    _posSlotDecoder[i].Init();
  }
  { 
    for(UInt32 i = 0; i < kNumFullDistances - kEndPosModelIndex; i++)
      _posDecoders[i].Init();
  }
  _posAlignDecoder.Init();
  _lenDecoder.Init(_posStateMask + 1);
  _repMatchLenDecoder.Init(_posStateMask + 1);
  _literalDecoder.Init();

  _state.Init();
  _reps[0] = _reps[1] = _reps[2] = _reps[3] = 0;
}

HRESULT CDecoder::CodeSpec(Byte *buffer, UInt32 curSize)
{
  if (_remainLen == -1)
    return S_OK;
  if (_remainLen == -2)
  {
    _rangeDecoder.Init();
    Init();
    _remainLen = 0;
  }
  if (curSize == 0)
    return S_OK;

  UInt64 nowPos64 = _nowPos64;

  UInt32 rep0 = _reps[0];
  UInt32 rep1 = _reps[1];
  UInt32 rep2 = _reps[2];
  UInt32 rep3 = _reps[3];
  CState state = _state;
  Byte previousByte;

  while(_remainLen > 0 && curSize > 0)
  {
    previousByte = _outWindowStream.GetByte(rep0);
    _outWindowStream.PutByte(previousByte);
    if (buffer)
      *buffer++ = previousByte;
    nowPos64++;
    _remainLen--;
    curSize--;
  }
  if (nowPos64 == 0)
    previousByte = 0;
  else
    previousByte = _outWindowStream.GetByte(0);

  while(curSize > 0)
  {
    {
      #ifdef _NO_EXCEPTIONS
      if (_rangeDecoder.Stream.ErrorCode != S_OK)
        return _rangeDecoder.Stream.ErrorCode;
      #endif
      if (_rangeDecoder.Stream.WasFinished())
        return S_FALSE;
      UInt32 posState = UInt32(nowPos64) & _posStateMask;
      if (_isMatch[state.Index][posState].Decode(&_rangeDecoder) == 0)
      {
        if(!state.IsCharState())
          previousByte = _literalDecoder.DecodeWithMatchByte(&_rangeDecoder, 
              (UInt32)nowPos64, previousByte, _outWindowStream.GetByte(rep0));
        else
          previousByte = _literalDecoder.DecodeNormal(&_rangeDecoder, 
              (UInt32)nowPos64, previousByte);
        _outWindowStream.PutByte(previousByte);
        if (buffer)
          *buffer++ = previousByte;
        state.UpdateChar();
        curSize--;
        nowPos64++;
      }
      else             
      {
        UInt32 len;
        if(_isRep[state.Index].Decode(&_rangeDecoder) == 1)
        {
          if(_isRepG0[state.Index].Decode(&_rangeDecoder) == 0)
          {
            if(_isRep0Long[state.Index][posState].Decode(&_rangeDecoder) == 0)
            {
              if (nowPos64 == 0)
                return S_FALSE;
              state.UpdateShortRep();
              previousByte = _outWindowStream.GetByte(rep0);
              _outWindowStream.PutByte(previousByte);
              if (buffer)
                *buffer++ = previousByte;
              curSize--;
              nowPos64++;
              continue;
            }
          }
          else
          {
            UInt32 distance;
            if(_isRepG1[state.Index].Decode(&_rangeDecoder) == 0)
              distance = rep1;
            else 
            {
              if (_isRepG2[state.Index].Decode(&_rangeDecoder) == 0)
                distance = rep2;
              else
              {
                distance = rep3;
                rep3 = rep2;
              }
              rep2 = rep1;
            }
            rep1 = rep0;
            rep0 = distance;
          }
          len = _repMatchLenDecoder.Decode(&_rangeDecoder, posState) + kMatchMinLen;
          state.UpdateRep();
        }
        else
        {
          rep3 = rep2;
          rep2 = rep1;
          rep1 = rep0;
          len = kMatchMinLen + _lenDecoder.Decode(&_rangeDecoder, posState);
          state.UpdateMatch();
          UInt32 posSlot = _posSlotDecoder[GetLenToPosState(len)].Decode(&_rangeDecoder);
          if (posSlot >= kStartPosModelIndex)
          {
            UInt32 numDirectBits = (posSlot >> 1) - 1;
            rep0 = ((2 | (posSlot & 1)) << numDirectBits);

            if (posSlot < kEndPosModelIndex)
              rep0 += NRangeCoder::ReverseBitTreeDecode(_posDecoders + 
                  rep0 - posSlot - 1, &_rangeDecoder, numDirectBits);
            else
            {
              rep0 += (_rangeDecoder.DecodeDirectBits(
                  numDirectBits - kNumAlignBits) << kNumAlignBits);
              rep0 += _posAlignDecoder.ReverseDecode(&_rangeDecoder);
            }
          }
          else
            rep0 = posSlot;
          if (rep0 >= nowPos64 || rep0 >= _dictionarySizeCheck)
          {
            if (rep0 != (UInt32)(Int32)(-1))
              return S_FALSE;
            _nowPos64 = nowPos64;
            _remainLen = -1;
            return S_OK;
          }
        }
        UInt32 locLen = len;
        if (len > curSize)
          locLen = (UInt32)curSize;
        if (buffer)
        {
          for (UInt32 i = locLen; i != 0; i--)
          {
            previousByte = _outWindowStream.GetByte(rep0);
            *buffer++ = previousByte;
            _outWindowStream.PutByte(previousByte);
          }
        }
        else
        {
          _outWindowStream.CopyBlock(rep0, locLen);
          previousByte = _outWindowStream.GetByte(0);
        }
        curSize -= locLen;
        nowPos64 += locLen;
        len -= locLen;
        if (len != 0)
        {
          _remainLen = (Int32)len;
          break;
        }

        #ifdef _NO_EXCEPTIONS
        if (_outWindowStream.ErrorCode != S_OK)
          return _outWindowStream.ErrorCode;
        #endif
      }
    }
  }
  if (_rangeDecoder.Stream.WasFinished())
    return S_FALSE;
  _nowPos64 = nowPos64;
  _reps[0] = rep0;
  _reps[1] = rep1;
  _reps[2] = rep2;
  _reps[3] = rep3;
  _state = state;

  return S_OK;
}

STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, 
    const UInt64 *, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  SetInStream(inStream);
  _outWindowStream.SetStream(outStream);
  SetOutStreamSize(outSize);
  CDecoderFlusher flusher(this);

  while (true)
  {
    UInt32 curSize = 1 << 18;
    if (_outSize != (UInt64)(Int64)(-1))
      if (curSize > _outSize - _nowPos64)
        curSize = (UInt32)(_outSize - _nowPos64);
    RINOK(CodeSpec(0, curSize));
    if (_remainLen == -1)
      break;
    if (progress != NULL)
    {
      UInt64 inSize = _rangeDecoder.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&inSize, &_nowPos64));
    }
    if (_outSize != (UInt64)(Int64)(-1))
      if (_nowPos64 >= _outSize)
        break;
  } 
  flusher.NeedFlush = false;
  return Flush();
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress)
{
  #ifndef _NO_EXCEPTIONS
  try 
  { 
  #endif
    return CodeReal(inStream, outStream, inSize, outSize, progress); 
  #ifndef _NO_EXCEPTIONS
  }
  catch(const CInBufferException &e)  { return e.ErrorCode; }
  catch(const CLZOutWindowException &e)  { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
  #endif
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *properties, UInt32 size)
{
  if (size < 5)
    return E_INVALIDARG;
  int lc = properties[0] % 9;
  Byte remainder = (Byte)(properties[0] / 9);
  int lp = remainder % 5;
  int pb = remainder / 5;
  if (pb > NLength::kNumPosStatesBitsMax)
    return E_INVALIDARG;
  _posStateMask = (1 << pb) - 1;
  UInt32 dictionarySize = 0;
  for (int i = 0; i < 4; i++)
    dictionarySize += ((UInt32)(properties[1 + i])) << (i * 8);
  _dictionarySizeCheck = MyMax(dictionarySize, UInt32(1));
  UInt32 blockSize = MyMax(_dictionarySizeCheck, UInt32(1 << 12));
  if (!_outWindowStream.Create(blockSize))
    return E_OUTOFMEMORY;
  if (!_literalDecoder.Create(lp, lc))
    return E_OUTOFMEMORY;
  if (!_rangeDecoder.Create(1 << 20))
    return E_OUTOFMEMORY;
  return S_OK;
}

STDMETHODIMP CDecoder::GetInStreamProcessedSize(UInt64 *value)
{
  *value = _rangeDecoder.GetProcessedSize();
  return S_OK;
}

STDMETHODIMP CDecoder::SetInStream(ISequentialInStream *inStream)
{
  _rangeDecoder.SetStream(inStream);
  return S_OK;
}

STDMETHODIMP CDecoder::ReleaseInStream()
{
  _rangeDecoder.ReleaseStream();
  return S_OK;
}

STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 *outSize)
{
  _outSize = (outSize == NULL) ? (UInt64)(Int64)(-1) : *outSize;
  _nowPos64 = 0;
  _remainLen = -2; // -2 means need_init
  _outWindowStream.Init();
  return S_OK;
}

STDMETHODIMP CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  #ifndef _NO_EXCEPTIONS
  try 
  { 
  #endif
  UInt64 startPos = _nowPos64;
  if (_outSize != (UInt64)(Int64)(-1))
    if (size > _outSize - _nowPos64)
      size = (UInt32)(_outSize - _nowPos64);
  HRESULT res = CodeSpec((Byte *)data, size);
  if (processedSize)
    *processedSize = (UInt32)(_nowPos64 - startPos);
  return res;
  #ifndef _NO_EXCEPTIONS
  }
  catch(const CInBufferException &e)  { return e.ErrorCode; }
  catch(const CLZOutWindowException &e)  { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
  #endif
}

STDMETHODIMP CDecoder::ReadPart(void *data, UInt32 size, UInt32 *processedSize)
{
  return Read(data, size, processedSize);
}

}}
