// CoderMixer2.h

#pragma once

#ifndef __CODERMIXER2_H
#define __CODERMIXER2_H

#include "Util/StreamBinder.h"
#include "../../../Compress/Interface/CompressInterface.h"
#include "Common/Vector.h"
#include "Windows/Thread.h"

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
  
  UINT32 GetCoderStartOutStream(UINT32 aCoderIndex) const
  {
    UINT32 aNumOutStreams = 0;
    for (int i = 0; i < aCoderIndex; i++)
      aNumOutStreams += CodersInfo[i].NumOutStreams;
    return aNumOutStreams;
  }


  void GetNumStreams(UINT32 &aNumInStreams, UINT32 &aNumOutStreams) const
  {
    aNumInStreams = 0;
    aNumOutStreams = 0;
    for (int i = 0; i < CodersInfo.Size(); i++)
    {
      const CCoderStreamsInfo &aCoderStreamsInfo = CodersInfo[i];
      aNumInStreams += aCoderStreamsInfo.NumInStreams;
      aNumOutStreams += aCoderStreamsInfo.NumOutStreams;
    }
  }

  int FindBinderForInStream(UINT32 anInStream) const
  {
    for (int i = 0; i < BindPairs.Size(); i++)
      if (BindPairs[i].InIndex == anInStream)
        return i;
    return -1;
  }
  int FindBinderForOutStream(UINT32 anOutStream) const
  {
    for (int i = 0; i < BindPairs.Size(); i++)
      if (BindPairs[i].OutIndex == anOutStream)
        return i;
    return -1;
  }

  UINT32 GetCoderInStreamIndex(UINT32 aCoderIndex) const
  {
    UINT32 aStreamIndex = 0;
    for (UINT32 i = 0; i < aCoderIndex; i++)
      aStreamIndex += CodersInfo[i].NumInStreams;
    return aStreamIndex;
  }

  UINT32 GetCoderOutStreamIndex(UINT32 aCoderIndex) const
  {
    UINT32 aStreamIndex = 0;
    for (UINT32 i = 0; i < aCoderIndex; i++)
      aStreamIndex += CodersInfo[i].NumOutStreams;
    return aStreamIndex;
  }


  void FindInStream(UINT32 aStreamIndex, UINT32 &aCoderIndex, 
      UINT32 &aCoderStreamIndex) const
  {
    for (aCoderIndex = 0; aCoderIndex < CodersInfo.Size(); aCoderIndex++)
    {
      UINT32 aCurSize = CodersInfo[aCoderIndex].NumInStreams;
      if (aStreamIndex < aCurSize)
      {
        aCoderStreamIndex = aStreamIndex;
        return;
      }
      aStreamIndex -= aCurSize;
    }
    throw 1;
  }
  void FindOutStream(UINT32 aStreamIndex, UINT32 &aCoderIndex, 
      UINT32 &aCoderStreamIndex) const
  {
    for (aCoderIndex = 0; aCoderIndex < CodersInfo.Size(); aCoderIndex++)
    {
      UINT32 aCurSize = CodersInfo[aCoderIndex].NumOutStreams;
      if (aStreamIndex < aCurSize)
      {
        aCoderStreamIndex = aStreamIndex;
        return;
      }
      aStreamIndex -= aCurSize;
    }
    throw 1;
  }
};

class CBindReverseConverter
{
public:
  UINT32 m_NumSrcInStreams;
  UINT32 m_NumSrcOutStreams;
  const NCoderMixer2::CBindInfo m_SrcBindInfo;
  CRecordVector<UINT32> m_SrcInToDestOutMap;
  CRecordVector<UINT32> m_SrcOutToDestInMap;
  CRecordVector<UINT32> m_DestInToSrcOutMap;
  CRecordVector<UINT32> m_DestOutToSrcInMap;
  CBindReverseConverter(const NCoderMixer2::CBindInfo &aSrcBindInfo);
  void CreateReverseBindInfo(NCoderMixer2::CBindInfo &aDestBindInfo);
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
  CComPtr<ICompressCoder> Coder;
  CComPtr<ICompressCoder2> Coder2;
  UINT32 NumInStreams;
  UINT32 NumOutStreams;

  CObjectVector< CComPtr<ISequentialInStream> > InStreams;
  CObjectVector< CComPtr<ISequentialOutStream> > OutStreams;
  CRecordVector<ISequentialInStream *> InStreamPointers;
  CRecordVector<ISequentialOutStream *> OutStreamPointers;
  CRecordVector<UINT64> InSizes;
  CRecordVector<UINT64> OutSizes;
  CRecordVector<const UINT64 *> InSizePointers;
  CRecordVector<const UINT64 *> OutSizePointers;

  CComPtr<ICompressProgressInfo> Progress; // CComPtr
  HRESULT Result;

  CThreadCoderInfo2(UINT32 aNumInStreams, UINT32 aNumOutStreams);
  void SetCoderInfo(const UINT64 **anInSizes,
      const UINT64 **anOutSizes, ICompressProgressInfo *aProgress);
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
//    SetProgressIndex(UINT32 aCoderIndex);
//    Code
//  }


class CCoderMixer2:
  public ICompressCoder2,
  public CComObjectRoot
{
BEGIN_COM_MAP(CCoderMixer2)
  COM_INTERFACE_ENTRY(ICompressCoder2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCoderMixer2)

DECLARE_NO_REGISTRY()

public:
  STDMETHOD(Init)(ISequentialInStream **anInStreams,
    ISequentialOutStream **anOutStreams);

  STDMETHOD(Code)(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes, 
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams, 
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress);


  CCoderMixer2();
  ~CCoderMixer2();
  void AddCoderCommon();
  void AddCoder(ICompressCoder *aCoder);
  void AddCoder2(ICompressCoder2 *aCoder);

  void ReInit();
  void SetCoderInfo(UINT32 aCoderIndex, const UINT64 **anInSize, const UINT64 **anOutSize)
    {  m_CoderInfoVector[aCoderIndex].SetCoderInfo(anInSize, anOutSize, NULL); }
  void SetProgressCoderIndex(UINT32 aCoderIndex)
    {  m_ProgressCoderIndex = aCoderIndex; }


  UINT64 GetWriteProcessedSize(UINT32 aBinderIndex) const;


  bool MyCode();

private:
  CBindInfo m_BindInfo;
  CObjectVector<CStreamBinder> m_StreamBinders;
  CObjectVector<CThreadCoderInfo2> m_CoderInfoVector;
  CRecordVector<HANDLE> m_Threads;
  NWindows::CThread m_MainThread;

  NWindows::NSynchronization::CAutoResetEvent m_StartCompressingEvent;
  CRecordVector<HANDLE> m_CompressingCompletedEvents;
  NWindows::NSynchronization::CAutoResetEvent m_CompressingFinishedEvent;

  NWindows::NSynchronization::CManualResetEvent ExitEvent;
  UINT32 m_ProgressCoderIndex;

public:
  void SetBindInfo(const CBindInfo &aBindInfo);

};

}
#endif

