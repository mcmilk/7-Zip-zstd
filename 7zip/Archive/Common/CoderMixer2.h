// CoderMixer2.h

#pragma once

#ifndef __CODERMIXER2_H
#define __CODERMIXER2_H

#include "../../../Common/Vector.h"
#include "../../../Common/MyCom.h"
#include "../../../Windows/Thread.h"
#include "../../Common/StreamBinder.h"
#include "../../ICoder.h"

// #include "../../../Compress/Interface/CompressInterface.h"

namespace NCoderMixer2 {

struct CBindPair
{
  UINT32 InIndex;
  UINT32 OutIndex;
};

struct CCoderStreamsInfo
{
  UINT32 NumInStreams;
  UINT32 NumOutStreams;
};

struct CBindInfo
{
  CRecordVector<CCoderStreamsInfo> CodersInfo;
  CRecordVector<CBindPair> BindPairs;
  CRecordVector<UINT32> InStreams;
  CRecordVector<UINT32> OutStreams;
  
  UINT32 GetCoderStartOutStream(UINT32 coderIndex) const
  {
    UINT32 numOutStreams = 0;
    for (UINT32 i = 0; i < coderIndex; i++)
      numOutStreams += CodersInfo[i].NumOutStreams;
    return numOutStreams;
  }


  void GetNumStreams(UINT32 &numInStreams, UINT32 &numOutStreams) const
  {
    numInStreams = 0;
    numOutStreams = 0;
    for (int i = 0; i < CodersInfo.Size(); i++)
    {
      const CCoderStreamsInfo &coderStreamsInfo = CodersInfo[i];
      numInStreams += coderStreamsInfo.NumInStreams;
      numOutStreams += coderStreamsInfo.NumOutStreams;
    }
  }

  int FindBinderForInStream(UINT32 inStream) const
  {
    for (int i = 0; i < BindPairs.Size(); i++)
      if (BindPairs[i].InIndex == inStream)
        return i;
    return -1;
  }
  int FindBinderForOutStream(UINT32 outStream) const
  {
    for (int i = 0; i < BindPairs.Size(); i++)
      if (BindPairs[i].OutIndex == outStream)
        return i;
    return -1;
  }

  UINT32 GetCoderInStreamIndex(UINT32 coderIndex) const
  {
    UINT32 streamIndex = 0;
    for (UINT32 i = 0; i < coderIndex; i++)
      streamIndex += CodersInfo[i].NumInStreams;
    return streamIndex;
  }

  UINT32 GetCoderOutStreamIndex(UINT32 coderIndex) const
  {
    UINT32 streamIndex = 0;
    for (UINT32 i = 0; i < coderIndex; i++)
      streamIndex += CodersInfo[i].NumOutStreams;
    return streamIndex;
  }


  void FindInStream(UINT32 streamIndex, UINT32 &coderIndex, 
      UINT32 &coderStreamIndex) const
  {
    for (coderIndex = 0; coderIndex < (UINT32)CodersInfo.Size(); coderIndex++)
    {
      UINT32 curSize = CodersInfo[coderIndex].NumInStreams;
      if (streamIndex < curSize)
      {
        coderStreamIndex = streamIndex;
        return;
      }
      streamIndex -= curSize;
    }
    throw 1;
  }
  void FindOutStream(UINT32 streamIndex, UINT32 &coderIndex, 
      UINT32 &coderStreamIndex) const
  {
    for (coderIndex = 0; coderIndex < (UINT32)CodersInfo.Size(); coderIndex++)
    {
      UINT32 curSize = CodersInfo[coderIndex].NumOutStreams;
      if (streamIndex < curSize)
      {
        coderStreamIndex = streamIndex;
        return;
      }
      streamIndex -= curSize;
    }
    throw 1;
  }
};

class CBindReverseConverter
{
  UINT32 _numSrcOutStreams;
  const NCoderMixer2::CBindInfo _srcBindInfo;
  CRecordVector<UINT32> _srcInToDestOutMap;
  CRecordVector<UINT32> _srcOutToDestInMap;
  CRecordVector<UINT32> _destInToSrcOutMap;
public:
  UINT32 NumSrcInStreams;
  CRecordVector<UINT32> DestOutToSrcInMap;

  CBindReverseConverter(const NCoderMixer2::CBindInfo &srcBindInfo);
  void CreateReverseBindInfo(NCoderMixer2::CBindInfo &destBindInfo);
};

//  CreateEvents();
//  {
//    SetCoderInfo()
//    Init Streams   
//    set CompressEvent()
//    wait CompressionCompletedEvent
//  }

struct CThreadCoderInfo2
{
  NWindows::NSynchronization::CAutoResetEvent *CompressEvent;
  HANDLE ExitEvent;
  NWindows::NSynchronization::CAutoResetEvent *CompressionCompletedEvent;

  bool CompressorIsCoder2;
  CMyComPtr<ICompressCoder> Coder;
  CMyComPtr<ICompressCoder2> Coder2;
  UINT32 NumInStreams;
  UINT32 NumOutStreams;

  CObjectVector< CMyComPtr<ISequentialInStream> > InStreams;
  CObjectVector< CMyComPtr<ISequentialOutStream> > OutStreams;
  CRecordVector<ISequentialInStream *> InStreamPointers;
  CRecordVector<ISequentialOutStream *> OutStreamPointers;
  CRecordVector<UINT64> InSizes;
  CRecordVector<UINT64> OutSizes;
  CRecordVector<const UINT64 *> InSizePointers;
  CRecordVector<const UINT64 *> OutSizePointers;

  CMyComPtr<ICompressProgressInfo> Progress; // CMyComPtr
  HRESULT Result;

  CThreadCoderInfo2(UINT32 numInStreams, UINT32 numOutStreams);
  void SetCoderInfo(const UINT64 **inSizes,
      const UINT64 **outSizes, ICompressProgressInfo *progress);
  ~CThreadCoderInfo2();
  bool WaitAndCode();
  void CreateEvents();
};


//  SetBindInfo()
//  for each coder
//  {
//    AddCoder[2]()
//  }
// 
//  for each file
//  {
//    ReInit()
//    for each coder
//    {
//      SetCoderInfo  
//    }
//    SetProgressIndex(UINT32 coderIndex);
//    Code
//  }


class CCoderMixer2:
  public ICompressCoder2,
  public CMyUnknownImp
{
  MY_UNKNOWN_IMP

public:
  STDMETHOD(Init)(ISequentialInStream **inStreams,
    ISequentialOutStream **outStreams);

  STDMETHOD(Code)(ISequentialInStream **inStreams,
      const UINT64 **inSizes, 
      UINT32 numInStreams,
      ISequentialOutStream **outStreams, 
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress);


  CCoderMixer2();
  ~CCoderMixer2();
  void AddCoderCommon();
  void AddCoder(ICompressCoder *coder);
  void AddCoder2(ICompressCoder2 *coder);

  void ReInit();
  void SetCoderInfo(UINT32 coderIndex, const UINT64 **anInSize, const UINT64 **outSize)
    {  _coderInfoVector[coderIndex].SetCoderInfo(anInSize, outSize, NULL); }
  void SetProgressCoderIndex(UINT32 coderIndex)
    {  _progressCoderIndex = coderIndex; }


  UINT64 GetWriteProcessedSize(UINT32 binderIndex) const;


  bool MyCode();

private:
  CBindInfo _bindInfo;
  CObjectVector<CStreamBinder> _streamBinders;
  CObjectVector<CThreadCoderInfo2> _coderInfoVector;
  CRecordVector<HANDLE> _threads;
  NWindows::CThread _mainThread;

  NWindows::NSynchronization::CAutoResetEvent _startCompressingEvent;
  CRecordVector<HANDLE> _compressingCompletedEvents;
  NWindows::NSynchronization::CAutoResetEvent _compressingFinishedEvent;

  NWindows::NSynchronization::CManualResetEvent _exitEvent;
  UINT32 _progressCoderIndex;

public:
  void SetBindInfo(const CBindInfo &bindInfo);

};

}
#endif

