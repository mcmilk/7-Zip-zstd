// CoderMixer2.cpp

#include "StdAfx.h"

#include "CoderMixer2.h"
#include "CrossThreadProgress.h"

using namespace NWindows;
using namespace NSynchronization;

//////////////////////////
// CThreadCoderInfo2

namespace NCoderMixer2 {

CBindReverseConverter::CBindReverseConverter(const CBindInfo &aSrcBindInfo):
  m_SrcBindInfo(aSrcBindInfo)
{
  aSrcBindInfo.GetNumStreams(m_NumSrcInStreams, m_NumSrcOutStreams);

  for (int j = 0; j < m_NumSrcInStreams; j++)
  {
    m_SrcInToDestOutMap.Add(0);
    m_DestOutToSrcInMap.Add(0);
  }
  for (j = 0; j < m_NumSrcOutStreams; j++)
  {
    m_SrcOutToDestInMap.Add(0);
    m_DestInToSrcOutMap.Add(0);
  }

  UINT32 aDestInOffset = 0;
  UINT32 aDestOutOffset = 0;
  UINT32 aSrcInOffset = m_NumSrcInStreams;
  UINT32 aSrcOutOffset = m_NumSrcOutStreams;

  for (int i = aSrcBindInfo.CodersInfo.Size() - 1; i >= 0; i--)
  {
    const CCoderStreamsInfo &aSrcCoderInfo = aSrcBindInfo.CodersInfo[i];

    aSrcInOffset -= aSrcCoderInfo.NumInStreams;
    aSrcOutOffset -= aSrcCoderInfo.NumOutStreams;

    for (int j = 0; j < aSrcCoderInfo.NumInStreams; j++, aDestOutOffset++)
    {
      UINT32 anIndex = aSrcInOffset + j;
      m_SrcInToDestOutMap[anIndex] = aDestOutOffset;
      m_DestOutToSrcInMap[aDestOutOffset] = anIndex;
    }
    for (j = 0; j < aSrcCoderInfo.NumOutStreams; j++, aDestInOffset++)
    {
      UINT32 anIndex = aSrcOutOffset + j;
      m_SrcOutToDestInMap[anIndex] = aDestInOffset;
      m_DestInToSrcOutMap[aDestInOffset] = anIndex;
    }
  }
}

void CBindReverseConverter::CreateReverseBindInfo(CBindInfo &aDestBindInfo)
{
  aDestBindInfo.CodersInfo.Clear();
  aDestBindInfo.BindPairs.Clear();
  aDestBindInfo.InStreams.Clear();
  aDestBindInfo.OutStreams.Clear();

  for (int i = m_SrcBindInfo.CodersInfo.Size() - 1; i >= 0; i--)
  {
    const CCoderStreamsInfo &aSrcCoderInfo = m_SrcBindInfo.CodersInfo[i];
    CCoderStreamsInfo aDestCoderInfo;
    aDestCoderInfo.NumInStreams = aSrcCoderInfo.NumOutStreams;
    aDestCoderInfo.NumOutStreams = aSrcCoderInfo.NumInStreams;
    aDestBindInfo.CodersInfo.Add(aDestCoderInfo);
  }
  for (i = m_SrcBindInfo.BindPairs.Size() - 1; i >= 0; i--)
  {
    const CBindPair &aSrcBindPair = m_SrcBindInfo.BindPairs[i];
    CBindPair aDestBindPair;
    aDestBindPair.InIndex = m_SrcOutToDestInMap[aSrcBindPair.OutIndex];
    aDestBindPair.OutIndex = m_SrcInToDestOutMap[aSrcBindPair.InIndex];
    aDestBindInfo.BindPairs.Add(aDestBindPair);
  }
  for (i = 0; i < m_SrcBindInfo.InStreams.Size(); i++)
    aDestBindInfo.OutStreams.Add(m_SrcInToDestOutMap[m_SrcBindInfo.InStreams[i]]);
  for (i = 0; i < m_SrcBindInfo.OutStreams.Size(); i++)
    aDestBindInfo.InStreams.Add(m_SrcOutToDestInMap[m_SrcBindInfo.OutStreams[i]]);
}


CThreadCoderInfo2::CThreadCoderInfo2(UINT32 aNumInStreams, UINT32 aNumOutStreams): 
    ExitEvent(NULL), 
    CompressEvent(NULL), 
    CompressionCompletedEvent(NULL), 
    NumInStreams(aNumInStreams),
    NumOutStreams(aNumOutStreams)
{
  InStreams.Reserve(NumInStreams);
  InStreamPointers.Reserve(NumInStreams);
  InSizes.Reserve(NumInStreams);
  InSizePointers.Reserve(NumInStreams);
  OutStreams.Reserve(NumOutStreams);
  OutStreamPointers.Reserve(NumOutStreams);
  OutSizes.Reserve(NumOutStreams);
  OutSizePointers.Reserve(NumOutStreams);
}

void CThreadCoderInfo2::CreateEvents()
{
  CompressEvent = new CAutoResetEvent(false);
  CompressionCompletedEvent = new CAutoResetEvent(false);
}

CThreadCoderInfo2::~CThreadCoderInfo2()
{
  if (CompressEvent != NULL)
    delete CompressEvent;
  if (CompressionCompletedEvent != NULL)
    delete CompressionCompletedEvent;
}

class CCoderInfoFlusher2
{
  CThreadCoderInfo2 *m_CoderInfo;
public:
  CCoderInfoFlusher2(CThreadCoderInfo2 *aCoderInfo): m_CoderInfo(aCoderInfo) {}
  ~CCoderInfoFlusher2()
  {
    for (int i = 0; i < m_CoderInfo->InStreams.Size(); i++)
      m_CoderInfo->InStreams[i].Release();
    for (i = 0; i < m_CoderInfo->OutStreams.Size(); i++)
      m_CoderInfo->OutStreams[i].Release();
    m_CoderInfo->CompressionCompletedEvent->Set();
  }
};

bool CThreadCoderInfo2::WaitAndCode()
{
  HANDLE anEvents[2] = { ExitEvent, *CompressEvent };
  DWORD anWaitResult = ::WaitForMultipleObjects(2, anEvents, FALSE, INFINITE);
  if (anWaitResult == WAIT_OBJECT_0 + 0)
    return false;

  {
    InStreamPointers.Clear();
    OutStreamPointers.Clear();
    for (int i = 0; i < NumInStreams; i++)
    {
      if (InSizePointers[i] != NULL)
        InSizePointers[i] = &InSizes[i];
      InStreamPointers.Add(InStreams[i]);
    }
    for (i = 0; i < NumOutStreams; i++)
    {
      if (OutSizePointers[i] != NULL)
        OutSizePointers[i] = &OutSizes[i];
      OutStreamPointers.Add(OutStreams[i]);
    }
    CCoderInfoFlusher2 aCoderInfoFlusher(this);
    if (CompressorIsCoder2)
      Result = Coder2->Code(&InStreamPointers.Front(),
        &InSizePointers.Front(),
        NumInStreams,
        &OutStreamPointers.Front(),
        &OutSizePointers.Front(),
        NumOutStreams,
        Progress);
    else
      Result = Coder->Code(InStreamPointers[0],
        OutStreamPointers[0],
        InSizePointers[0],
        OutSizePointers[0],
        Progress);
  }
  return true;
}

void SetSizes(const UINT64 **aSrcSizes, CRecordVector<UINT64> &aSizes, 
    CRecordVector<const UINT64 *> &aSizePointers, UINT32 aNumItems)
{
  aSizes.Clear();
  aSizePointers.Clear();
  for(UINT32 i = 0; i < aNumItems; i++)
  {
    if (aSrcSizes == 0 || aSrcSizes[i] == NULL)
    {
      aSizes.Add(0);
      aSizePointers.Add(NULL);
    }
    else
    {
      aSizes.Add(*aSrcSizes[i]);
      aSizePointers.Add(&aSizes.Back());
    }
  }
}


void CThreadCoderInfo2::SetCoderInfo(const UINT64 **anInSizes,
      const UINT64 **anOutSizes, ICompressProgressInfo *aProgress)
{
  Progress = aProgress;
  SetSizes(anInSizes, InSizes, InSizePointers, NumInStreams);
  SetSizes(anOutSizes, OutSizes, OutSizePointers, NumOutStreams);
}

static DWORD WINAPI CoderThread(void *aThreadCoderInfo)
{
  while(true)
  {
    if (!((CThreadCoderInfo2 *)aThreadCoderInfo)->WaitAndCode())
      return 0;
  }
}

//////////////////////////////////////
// CCoderMixer2

static DWORD WINAPI MainCoderThread(void *aThreadCoderInfo)
{
  while(true)
  {
    if (!((CCoderMixer2 *)aThreadCoderInfo)->MyCode())
      return 0;
  }
}

CCoderMixer2::CCoderMixer2()
{
  if (!m_MainThread.Create(MainCoderThread, this))
    throw 271825;
}

CCoderMixer2::~CCoderMixer2()
{
  ExitEvent.Set();

  ::WaitForSingleObject(m_MainThread, INFINITE);
  DWORD aResult = ::WaitForMultipleObjects(m_Threads.Size(), 
      &m_Threads.Front(), TRUE, INFINITE);
  for(int i = 0; i < m_Threads.Size(); i++)
    ::CloseHandle(m_Threads[i]);
}

void CCoderMixer2::SetBindInfo(const CBindInfo &aBindInfo)
{  
  m_BindInfo = aBindInfo; 
  m_StreamBinders.Clear();
  for(int i = 0; i < m_BindInfo.BindPairs.Size(); i++)
  {
    m_StreamBinders.Add(CStreamBinder());
    m_StreamBinders.Back().CreateEvents();
  }
}

void CCoderMixer2::AddCoderCommon()
{
  int anIndex = m_CoderInfoVector.Size();
  const CCoderStreamsInfo &CoderStreamsInfo = m_BindInfo.CodersInfo[anIndex];

  CThreadCoderInfo2 aThreadCoderInfo(CoderStreamsInfo.NumInStreams, 
      CoderStreamsInfo.NumOutStreams);
  m_CoderInfoVector.Add(aThreadCoderInfo);
  m_CoderInfoVector.Back().CreateEvents();
  m_CoderInfoVector.Back().ExitEvent = ExitEvent;
  m_CompressingCompletedEvents.Add(*m_CoderInfoVector.Back().CompressionCompletedEvent);

  DWORD anID;
  HANDLE aNewThread = ::CreateThread(NULL, 0, CoderThread, 
      &m_CoderInfoVector.Back(), 0, &anID);
  if (aNewThread == 0)
    throw 271824;
  m_Threads.Add(aNewThread);
}

void CCoderMixer2::AddCoder(ICompressCoder *aCoder)
{
  AddCoderCommon();
  m_CoderInfoVector.Back().CompressorIsCoder2 = false;
  m_CoderInfoVector.Back().Coder = aCoder;
}

void CCoderMixer2::AddCoder2(ICompressCoder2 *aCoder)
{
  AddCoderCommon();
  m_CoderInfoVector.Back().CompressorIsCoder2 = true;
  m_CoderInfoVector.Back().Coder2 = aCoder;
}

/*
void CCoderMixer2::FinishAddingCoders()
{
  for(int i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    DWORD anID;
    HANDLE aNewThread = ::CreateThread(NULL, 0, CoderThread, 
        &m_CoderInfoVector[i], 0, &anID);
    if (aNewThread == 0)
      throw 271824;
    m_Threads.Add(aNewThread);
  }
}
*/

void CCoderMixer2::ReInit()
{
  for(int i = 0; i < m_StreamBinders.Size(); i++)
    m_StreamBinders[i].ReInit();
}


STDMETHODIMP CCoderMixer2::Init(ISequentialInStream **anInStreams,
    ISequentialOutStream **anOutStreams) 
{
  if (m_CoderInfoVector.Size() != m_BindInfo.CodersInfo.Size())
    throw 0;
  UINT32 aNumInStreams = 0, aNumOutStreams = 0;
  for(int i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    CThreadCoderInfo2 &aCoderInfo = m_CoderInfoVector[i];
    const CCoderStreamsInfo &aCoderStreamsInfo = m_BindInfo.CodersInfo[i];
    aCoderInfo.InStreams.Clear();
    for(int j = 0; j < aCoderStreamsInfo.NumInStreams; j++)
      aCoderInfo.InStreams.Add(NULL);
    aCoderInfo.OutStreams.Clear();
    for(j = 0; j < aCoderStreamsInfo.NumOutStreams; j++)
      aCoderInfo.OutStreams.Add(NULL);
  }

  for(i = 0; i < m_BindInfo.BindPairs.Size(); i++)
  {
    const CBindPair &aBindPair = m_BindInfo.BindPairs[i];
    UINT32 anInCoderIndex, anInCoderStreamIndex;
    UINT32 anOutCoderIndex, anOutCoderStreamIndex;
    m_BindInfo.FindInStream(aBindPair.InIndex, anInCoderIndex, anInCoderStreamIndex);
    m_BindInfo.FindOutStream(aBindPair.OutIndex, anOutCoderIndex, anOutCoderStreamIndex);

    m_StreamBinders[i].CreateStreams(
        &m_CoderInfoVector[anInCoderIndex].InStreams[anInCoderStreamIndex],
        &m_CoderInfoVector[anOutCoderIndex].OutStreams[anOutCoderStreamIndex]);
  }

  for(i = 0; i < m_BindInfo.InStreams.Size(); i++)
  {
    UINT32 anInCoderIndex, anInCoderStreamIndex;
    m_BindInfo.FindInStream(m_BindInfo.InStreams[i], anInCoderIndex, anInCoderStreamIndex);
    m_CoderInfoVector[anInCoderIndex].InStreams[anInCoderStreamIndex] = anInStreams[i];
  }
  
  for(i = 0; i < m_BindInfo.OutStreams.Size(); i++)
  {
    UINT32 anOutCoderIndex, anOutCoderStreamIndex;
    m_BindInfo.FindOutStream(m_BindInfo.OutStreams[i], anOutCoderIndex, anOutCoderStreamIndex);
    m_CoderInfoVector[anOutCoderIndex].OutStreams[anOutCoderStreamIndex] = anOutStreams[i];
  }
  return S_OK;
}


bool CCoderMixer2::MyCode()
{
  HANDLE anEvents[2] = { ExitEvent, m_StartCompressingEvent };
  DWORD anWaitResult = ::WaitForMultipleObjects(2, anEvents, FALSE, INFINITE);
  if (anWaitResult == WAIT_OBJECT_0 + 0)
    return false;

  for(int i = 0; i < m_CoderInfoVector.Size(); i++)
    m_CoderInfoVector[i].CompressEvent->Set();
  DWORD aResult = ::WaitForMultipleObjects(m_CompressingCompletedEvents.Size(), 
      &m_CompressingCompletedEvents.Front(), TRUE, INFINITE);
  
  m_CompressingFinishedEvent.Set();

  return true;
}


STDMETHODIMP CCoderMixer2::Code(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes, 
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams, 
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress)
{
  if (aNumInStreams != m_BindInfo.InStreams.Size() ||
      aNumOutStreams != m_BindInfo.OutStreams.Size())
    return E_INVALIDARG;

  Init(anInStreams, anOutStreams);

  m_CompressingFinishedEvent.Reset(); // ?
  
  CComObjectNoLock<CCrossThreadProgress> *aProgressSpec = 
      new CComObjectNoLock<CCrossThreadProgress>;
  CComPtr<ICompressProgressInfo> aCrossProgress = aProgressSpec;
  aProgressSpec->Init();
  m_CoderInfoVector[m_ProgressCoderIndex].Progress = aCrossProgress;

  m_StartCompressingEvent.Set();


  while (true)
  {
    HANDLE anEvents[2] = {m_CompressingFinishedEvent, aProgressSpec->m_ProgressEvent };
    DWORD anWaitResult = ::WaitForMultipleObjects(2, anEvents, FALSE, INFINITE);
    if (anWaitResult == WAIT_OBJECT_0 + 0)
      break;
    if (aProgress != NULL)
      aProgressSpec->m_Result = aProgress->SetRatioInfo(aProgressSpec->m_InSize, 
          aProgressSpec->m_OutSize);
    else
      aProgressSpec->m_Result = S_OK;
    aProgressSpec->m_WaitEvent.Set();
  }


  for(int i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    HRESULT aResult = m_CoderInfoVector[i].Result;
    if (aResult == S_FALSE)
      return aResult;
  }
  for(i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    HRESULT aResult = m_CoderInfoVector[i].Result;
    if (aResult != S_OK && aResult != E_FAIL)
      return aResult;
  }
  for(i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    HRESULT aResult = m_CoderInfoVector[i].Result;
    if (aResult != S_OK)
      return aResult;
  }
  return S_OK;
}

UINT64 CCoderMixer2::GetWriteProcessedSize(UINT32 aBinderIndex) const
{
  return m_StreamBinders[aBinderIndex].m_ProcessedSize;
}

}  
