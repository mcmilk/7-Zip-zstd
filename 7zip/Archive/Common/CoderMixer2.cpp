// CoderMixer2.cpp

#include "StdAfx.h"

#include "CoderMixer2.h"
#include "CrossThreadProgress.h"

using namespace NWindows;
using namespace NSynchronization;

//////////////////////////
// CThreadCoderInfo2

namespace NCoderMixer2 {

CBindReverseConverter::CBindReverseConverter(const CBindInfo &srcBindInfo):
  _srcBindInfo(srcBindInfo)
{
  srcBindInfo.GetNumStreams(NumSrcInStreams, _numSrcOutStreams);

  UINT32  j;
  for (j = 0; j < NumSrcInStreams; j++)
  {
    _srcInToDestOutMap.Add(0);
    DestOutToSrcInMap.Add(0);
  }
  for (j = 0; j < _numSrcOutStreams; j++)
  {
    _srcOutToDestInMap.Add(0);
    _destInToSrcOutMap.Add(0);
  }

  UINT32 destInOffset = 0;
  UINT32 destOutOffset = 0;
  UINT32 srcInOffset = NumSrcInStreams;
  UINT32 srcOutOffset = _numSrcOutStreams;

  for (int i = srcBindInfo.CodersInfo.Size() - 1; i >= 0; i--)
  {
    const CCoderStreamsInfo &srcCoderInfo = srcBindInfo.CodersInfo[i];

    srcInOffset -= srcCoderInfo.NumInStreams;
    srcOutOffset -= srcCoderInfo.NumOutStreams;
    
    UINT32 j;
    for (j = 0; j < srcCoderInfo.NumInStreams; j++, destOutOffset++)
    {
      UINT32 index = srcInOffset + j;
      _srcInToDestOutMap[index] = destOutOffset;
      DestOutToSrcInMap[destOutOffset] = index;
    }
    for (j = 0; j < srcCoderInfo.NumOutStreams; j++, destInOffset++)
    {
      UINT32 index = srcOutOffset + j;
      _srcOutToDestInMap[index] = destInOffset;
      _destInToSrcOutMap[destInOffset] = index;
    }
  }
}

void CBindReverseConverter::CreateReverseBindInfo(CBindInfo &destBindInfo)
{
  destBindInfo.CodersInfo.Clear();
  destBindInfo.BindPairs.Clear();
  destBindInfo.InStreams.Clear();
  destBindInfo.OutStreams.Clear();

  int i;
  for (i = _srcBindInfo.CodersInfo.Size() - 1; i >= 0; i--)
  {
    const CCoderStreamsInfo &srcCoderInfo = _srcBindInfo.CodersInfo[i];
    CCoderStreamsInfo destCoderInfo;
    destCoderInfo.NumInStreams = srcCoderInfo.NumOutStreams;
    destCoderInfo.NumOutStreams = srcCoderInfo.NumInStreams;
    destBindInfo.CodersInfo.Add(destCoderInfo);
  }
  for (i = _srcBindInfo.BindPairs.Size() - 1; i >= 0; i--)
  {
    const CBindPair &srcBindPair = _srcBindInfo.BindPairs[i];
    CBindPair destBindPair;
    destBindPair.InIndex = _srcOutToDestInMap[srcBindPair.OutIndex];
    destBindPair.OutIndex = _srcInToDestOutMap[srcBindPair.InIndex];
    destBindInfo.BindPairs.Add(destBindPair);
  }
  for (i = 0; i < _srcBindInfo.InStreams.Size(); i++)
    destBindInfo.OutStreams.Add(_srcInToDestOutMap[_srcBindInfo.InStreams[i]]);
  for (i = 0; i < _srcBindInfo.OutStreams.Size(); i++)
    destBindInfo.InStreams.Add(_srcOutToDestInMap[_srcBindInfo.OutStreams[i]]);
}


CThreadCoderInfo2::CThreadCoderInfo2(UINT32 numInStreams, UINT32 numOutStreams): 
    ExitEvent(NULL), 
    CompressEvent(NULL), 
    CompressionCompletedEvent(NULL), 
    NumInStreams(numInStreams),
    NumOutStreams(numOutStreams)
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
  CCoderInfoFlusher2(CThreadCoderInfo2 *coderInfo): m_CoderInfo(coderInfo) {}
  ~CCoderInfoFlusher2()
  {
	  int i;
    for (i = 0; i < m_CoderInfo->InStreams.Size(); i++)
      m_CoderInfo->InStreams[i].Release();
    for (i = 0; i < m_CoderInfo->OutStreams.Size(); i++)
      m_CoderInfo->OutStreams[i].Release();
    m_CoderInfo->CompressionCompletedEvent->Set();
  }
};

bool CThreadCoderInfo2::WaitAndCode()
{
  HANDLE events[2] = { ExitEvent, *CompressEvent };
  DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
  if (waitResult == WAIT_OBJECT_0 + 0)
    return false;

  {
    InStreamPointers.Clear();
    OutStreamPointers.Clear();
    UINT32 i;
    for (i = 0; i < NumInStreams; i++)
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
    CCoderInfoFlusher2 coderInfoFlusher(this);
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

void SetSizes(const UINT64 **srcSizes, CRecordVector<UINT64> &sizes, 
    CRecordVector<const UINT64 *> &sizePointers, UINT32 numItems)
{
  sizes.Clear();
  sizePointers.Clear();
  for(UINT32 i = 0; i < numItems; i++)
  {
    if (srcSizes == 0 || srcSizes[i] == NULL)
    {
      sizes.Add(0);
      sizePointers.Add(NULL);
    }
    else
    {
      sizes.Add(*srcSizes[i]);
      sizePointers.Add(&sizes.Back());
    }
  }
}


void CThreadCoderInfo2::SetCoderInfo(const UINT64 **inSizes,
      const UINT64 **outSizes, ICompressProgressInfo *progress)
{
  Progress = progress;
  SetSizes(inSizes, InSizes, InSizePointers, NumInStreams);
  SetSizes(outSizes, OutSizes, OutSizePointers, NumOutStreams);
}

static DWORD WINAPI CoderThread(void *threadCoderInfo)
{
  while(true)
  {
    if (!((CThreadCoderInfo2 *)threadCoderInfo)->WaitAndCode())
      return 0;
  }
}

//////////////////////////////////////
// CCoderMixer2

static DWORD WINAPI MainCoderThread(void *threadCoderInfo)
{
  while(true)
  {
    if (!((CCoderMixer2 *)threadCoderInfo)->MyCode())
      return 0;
  }
}

CCoderMixer2::CCoderMixer2()
{
  if (!_mainThread.Create(MainCoderThread, this))
    throw 271825;
}

CCoderMixer2::~CCoderMixer2()
{
  _exitEvent.Set();

  ::WaitForSingleObject(_mainThread, INFINITE);
  DWORD result = ::WaitForMultipleObjects(_threads.Size(), 
      &_threads.Front(), TRUE, INFINITE);
  for(int i = 0; i < _threads.Size(); i++)
    ::CloseHandle(_threads[i]);
}

void CCoderMixer2::SetBindInfo(const CBindInfo &bindInfo)
{  
  _bindInfo = bindInfo; 
  _streamBinders.Clear();
  for(int i = 0; i < _bindInfo.BindPairs.Size(); i++)
  {
    _streamBinders.Add(CStreamBinder());
    _streamBinders.Back().CreateEvents();
  }
}

void CCoderMixer2::AddCoderCommon()
{
  int index = _coderInfoVector.Size();
  const CCoderStreamsInfo &CoderStreamsInfo = _bindInfo.CodersInfo[index];

  CThreadCoderInfo2 threadCoderInfo(CoderStreamsInfo.NumInStreams, 
      CoderStreamsInfo.NumOutStreams);
  _coderInfoVector.Add(threadCoderInfo);
  _coderInfoVector.Back().CreateEvents();
  _coderInfoVector.Back().ExitEvent = _exitEvent;
  _compressingCompletedEvents.Add(*_coderInfoVector.Back().CompressionCompletedEvent);

  DWORD id;
  HANDLE newThread = ::CreateThread(NULL, 0, CoderThread, 
      &_coderInfoVector.Back(), 0, &id);
  if (newThread == 0)
    throw 271824;
  _threads.Add(newThread);
}

void CCoderMixer2::AddCoder(ICompressCoder *coder)
{
  AddCoderCommon();
  _coderInfoVector.Back().CompressorIsCoder2 = false;
  _coderInfoVector.Back().Coder = coder;
}

void CCoderMixer2::AddCoder2(ICompressCoder2 *coder)
{
  AddCoderCommon();
  _coderInfoVector.Back().CompressorIsCoder2 = true;
  _coderInfoVector.Back().Coder2 = coder;
}

/*
void CCoderMixer2::FinishAddingCoders()
{
  for(int i = 0; i < _coderInfoVector.Size(); i++)
  {
    DWORD id;
    HANDLE newThread = ::CreateThread(NULL, 0, CoderThread, 
        &_coderInfoVector[i], 0, &id);
    if (newThread == 0)
      throw 271824;
    _threads.Add(newThread);
  }
}
*/

void CCoderMixer2::ReInit()
{
  for(int i = 0; i < _streamBinders.Size(); i++)
    _streamBinders[i].ReInit();
}


STDMETHODIMP CCoderMixer2::Init(ISequentialInStream **inStreams,
    ISequentialOutStream **outStreams) 
{
  if (_coderInfoVector.Size() != _bindInfo.CodersInfo.Size())
    throw 0;
  UINT32 numInStreams = 0, numOutStreams = 0;
  int i;
  for(i = 0; i < _coderInfoVector.Size(); i++)
  {
    CThreadCoderInfo2 &coderInfo = _coderInfoVector[i];
    const CCoderStreamsInfo &coderStreamsInfo = _bindInfo.CodersInfo[i];
    coderInfo.InStreams.Clear();
    UINT32 j;
    for(j = 0; j < coderStreamsInfo.NumInStreams; j++)
      coderInfo.InStreams.Add(NULL);
    coderInfo.OutStreams.Clear();
    for(j = 0; j < coderStreamsInfo.NumOutStreams; j++)
      coderInfo.OutStreams.Add(NULL);
  }

  for(i = 0; i < _bindInfo.BindPairs.Size(); i++)
  {
    const CBindPair &bindPair = _bindInfo.BindPairs[i];
    UINT32 inCoderIndex, inCoderStreamIndex;
    UINT32 outCoderIndex, outCoderStreamIndex;
    _bindInfo.FindInStream(bindPair.InIndex, inCoderIndex, inCoderStreamIndex);
    _bindInfo.FindOutStream(bindPair.OutIndex, outCoderIndex, outCoderStreamIndex);

    _streamBinders[i].CreateStreams(
        &_coderInfoVector[inCoderIndex].InStreams[inCoderStreamIndex],
        &_coderInfoVector[outCoderIndex].OutStreams[outCoderStreamIndex]);
  }

  for(i = 0; i < _bindInfo.InStreams.Size(); i++)
  {
    UINT32 inCoderIndex, inCoderStreamIndex;
    _bindInfo.FindInStream(_bindInfo.InStreams[i], inCoderIndex, inCoderStreamIndex);
    _coderInfoVector[inCoderIndex].InStreams[inCoderStreamIndex] = inStreams[i];
  }
  
  for(i = 0; i < _bindInfo.OutStreams.Size(); i++)
  {
    UINT32 outCoderIndex, outCoderStreamIndex;
    _bindInfo.FindOutStream(_bindInfo.OutStreams[i], outCoderIndex, outCoderStreamIndex);
    _coderInfoVector[outCoderIndex].OutStreams[outCoderStreamIndex] = outStreams[i];
  }
  return S_OK;
}


bool CCoderMixer2::MyCode()
{
  HANDLE events[2] = { _exitEvent, _startCompressingEvent };
  DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
  if (waitResult == WAIT_OBJECT_0 + 0)
    return false;

  for(int i = 0; i < _coderInfoVector.Size(); i++)
    _coderInfoVector[i].CompressEvent->Set();
  DWORD result = ::WaitForMultipleObjects(_compressingCompletedEvents.Size(), 
      &_compressingCompletedEvents.Front(), TRUE, INFINITE);
  
  _compressingFinishedEvent.Set();

  return true;
}


STDMETHODIMP CCoderMixer2::Code(ISequentialInStream **inStreams,
      const UINT64 **inSizes, 
      UINT32 numInStreams,
      ISequentialOutStream **outStreams, 
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress)
{
  if (numInStreams != (UINT32)_bindInfo.InStreams.Size() ||
      numOutStreams != (UINT32)_bindInfo.OutStreams.Size())
    return E_INVALIDARG;

  Init(inStreams, outStreams);

  _compressingFinishedEvent.Reset(); // ?
  
  CCrossThreadProgress *progressSpec = new CCrossThreadProgress;
  CMyComPtr<ICompressProgressInfo> crossProgress = progressSpec;
  progressSpec->Init();
  _coderInfoVector[_progressCoderIndex].Progress = crossProgress;

  _startCompressingEvent.Set();


  while (true)
  {
    HANDLE events[2] = {_compressingFinishedEvent, progressSpec->ProgressEvent };
    DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
    if (waitResult == WAIT_OBJECT_0 + 0)
      break;
    if (progress != NULL)
      progressSpec->Result = progress->SetRatioInfo(progressSpec->InSize, 
          progressSpec->OutSize);
    else
      progressSpec->Result = S_OK;
    progressSpec->WaitEvent.Set();
  }

  int i;
  for(i = 0; i < _coderInfoVector.Size(); i++)
  {
    HRESULT result = _coderInfoVector[i].Result;
    if (result == S_FALSE)
      return result;
  }
  for(i = 0; i < _coderInfoVector.Size(); i++)
  {
    HRESULT result = _coderInfoVector[i].Result;
    if (result != S_OK && result != E_FAIL)
      return result;
  }
  for(i = 0; i < _coderInfoVector.Size(); i++)
  {
    HRESULT result = _coderInfoVector[i].Result;
    if (result != S_OK)
      return result;
  }
  return S_OK;
}

UINT64 CCoderMixer2::GetWriteProcessedSize(UINT32 binderIndex) const
{
  return _streamBinders[binderIndex].ProcessedSize;
}

}  
