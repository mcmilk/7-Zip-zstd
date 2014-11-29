// CoderMixer2.cpp

#include "StdAfx.h"

#include "CoderMixer2.h"

namespace NCoderMixer {

CBindReverseConverter::CBindReverseConverter(const CBindInfo &srcBindInfo):
  _srcBindInfo(srcBindInfo)
{
  srcBindInfo.GetNumStreams(NumSrcInStreams, _numSrcOutStreams);

  UInt32 j;
  _srcInToDestOutMap.ClearAndSetSize(NumSrcInStreams);
  DestOutToSrcInMap.ClearAndSetSize(NumSrcInStreams);

  for (j = 0; j < NumSrcInStreams; j++)
  {
    _srcInToDestOutMap[j] = 0;
    DestOutToSrcInMap[j] = 0;
  }

  _srcOutToDestInMap.ClearAndSetSize(_numSrcOutStreams);
  _destInToSrcOutMap.ClearAndSetSize(_numSrcOutStreams);

  for (j = 0; j < _numSrcOutStreams; j++)
  {
    _srcOutToDestInMap[j] = 0;
    _destInToSrcOutMap[j] = 0;
  }

  UInt32 destInOffset = 0;
  UInt32 destOutOffset = 0;
  UInt32 srcInOffset = NumSrcInStreams;
  UInt32 srcOutOffset = _numSrcOutStreams;

  for (int i = srcBindInfo.Coders.Size() - 1; i >= 0; i--)
  {
    const CCoderStreamsInfo &srcCoderInfo = srcBindInfo.Coders[i];

    srcInOffset -= srcCoderInfo.NumInStreams;
    srcOutOffset -= srcCoderInfo.NumOutStreams;
    
    UInt32 j;
    for (j = 0; j < srcCoderInfo.NumInStreams; j++, destOutOffset++)
    {
      UInt32 index = srcInOffset + j;
      _srcInToDestOutMap[index] = destOutOffset;
      DestOutToSrcInMap[destOutOffset] = index;
    }
    for (j = 0; j < srcCoderInfo.NumOutStreams; j++, destInOffset++)
    {
      UInt32 index = srcOutOffset + j;
      _srcOutToDestInMap[index] = destInOffset;
      _destInToSrcOutMap[destInOffset] = index;
    }
  }
}

void CBindReverseConverter::CreateReverseBindInfo(CBindInfo &destBindInfo)
{
  destBindInfo.Coders.ClearAndReserve(_srcBindInfo.Coders.Size());
  destBindInfo.BindPairs.ClearAndReserve(_srcBindInfo.BindPairs.Size());
  destBindInfo.InStreams.ClearAndReserve(_srcBindInfo.OutStreams.Size());
  destBindInfo.OutStreams.ClearAndReserve(_srcBindInfo.InStreams.Size());

  unsigned i;
  for (i = _srcBindInfo.Coders.Size(); i != 0;)
  {
    i--;
    const CCoderStreamsInfo &srcCoderInfo = _srcBindInfo.Coders[i];
    CCoderStreamsInfo destCoderInfo;
    destCoderInfo.NumInStreams = srcCoderInfo.NumOutStreams;
    destCoderInfo.NumOutStreams = srcCoderInfo.NumInStreams;
    destBindInfo.Coders.AddInReserved(destCoderInfo);
  }
  for (i = _srcBindInfo.BindPairs.Size(); i != 0;)
  {
    i--;
    const CBindPair &srcBindPair = _srcBindInfo.BindPairs[i];
    CBindPair destBindPair;
    destBindPair.InIndex = _srcOutToDestInMap[srcBindPair.OutIndex];
    destBindPair.OutIndex = _srcInToDestOutMap[srcBindPair.InIndex];
    destBindInfo.BindPairs.AddInReserved(destBindPair);
  }
  for (i = 0; i < _srcBindInfo.InStreams.Size(); i++)
    destBindInfo.OutStreams.AddInReserved(_srcInToDestOutMap[_srcBindInfo.InStreams[i]]);
  for (i = 0; i < _srcBindInfo.OutStreams.Size(); i++)
    destBindInfo.InStreams.AddInReserved(_srcOutToDestInMap[_srcBindInfo.OutStreams[i]]);
}

void SetSizes(const UInt64 **srcSizes, CRecordVector<UInt64> &sizes,
    CRecordVector<const UInt64 *> &sizePointers, UInt32 numItems)
{
  sizes.ClearAndSetSize(numItems);
  sizePointers.ClearAndSetSize(numItems);
  for(UInt32 i = 0; i < numItems; i++)
  {
    if (!srcSizes || !srcSizes[i])
    {
      sizes[i] = 0;
      sizePointers[i] = NULL;
    }
    else
    {
      sizes[i] = *(srcSizes[i]);
      sizePointers[i] = &sizes[i];
    }
  }
}

void CCoderInfo2::SetCoderInfo(const UInt64 **inSizes, const UInt64 **outSizes)
{
  SetSizes(inSizes, InSizes, InSizePointers, NumInStreams);
  SetSizes(outSizes, OutSizes, OutSizePointers, NumOutStreams);
}

}
