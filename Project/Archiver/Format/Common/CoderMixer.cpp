// CoderMixer.cpp

#include "StdAfx.h"

#include "CoderMixer.h"
#include "Common/Defs.h"
#include "CrossThreadProgress.h"

using namespace NWindows;
using namespace NSynchronization;

//////////////////////////
// CThreadCoderInfo

void CThreadCoderInfo::CreateEvents()
{
  CompressEvent = new CAutoResetEvent(false);
  CompressionCompletedEvent = new CAutoResetEvent(false);
}

CThreadCoderInfo::~CThreadCoderInfo()
{
  if (CompressEvent != NULL)
    delete CompressEvent;
  if (CompressionCompletedEvent != NULL)
    delete CompressionCompletedEvent;
}

class CCoderInfoFlusher
{
  CThreadCoderInfo *m_CoderInfo;
public:
  CCoderInfoFlusher(CThreadCoderInfo *aCoderInfo): m_CoderInfo(aCoderInfo) {}
  ~CCoderInfoFlusher()
  {
    m_CoderInfo->InStream.Release();
    m_CoderInfo->OutStream.Release();
    m_CoderInfo->CompressionCompletedEvent->Set();
  }
};

bool CThreadCoderInfo::WaitAndCode()
{
  HANDLE anEvents[2] = { ExitEvent, *CompressEvent };
  DWORD anWaitResult = ::WaitForMultipleObjects(2, anEvents, FALSE, INFINITE);
  if (anWaitResult == WAIT_OBJECT_0 + 0)
    return false;

  {
    CCoderInfoFlusher aCoderInfoFlusher(this);
    Result = Coder->Code(InStream, OutStream, 
        InSizeAssigned ? &InSizeValue : NULL, 
        OutSizeAssigned ? &OutSizeValue : NULL, 
        Progress);
  }
  return true;
}


DWORD WINAPI CoderThread(void *aThreadCoderInfo)
{
  while(true)
  {
    if (!((CThreadCoderInfo *)aThreadCoderInfo)->WaitAndCode())
      return 0;
  }
}

//////////////////////////
// CCoderMixer

static DWORD WINAPI MainCoderThread(void *aThreadCoderInfo)
{
  while(true)
  {
    if (!((CCoderMixer *)aThreadCoderInfo)->MyCode())
      return 0;
  }
}

CCoderMixer::CCoderMixer()
{
  if (!m_MainThread.Create(MainCoderThread, this))
    throw 271825;
}

CCoderMixer::~CCoderMixer()
{
  ExitEvent.Set();
  ::WaitForSingleObject(m_MainThread, INFINITE);
  DWORD result = ::WaitForMultipleObjects(m_Threads.Size(), 
      (const HANDLE *)m_Threads.GetPointer(),  TRUE, INFINITE);
  for(int i = 0; i < m_Threads.Size(); i++)
    ::CloseHandle(m_Threads[i]);
}


void CCoderMixer::AddCoder(ICompressCoder *aCoder)
{
  CThreadCoderInfo aThreadCoderInfo;
  aThreadCoderInfo.Coder = aCoder;
  m_CoderInfoVector.Add(aThreadCoderInfo);
  m_CoderInfoVector.Back().CreateEvents();
  m_CoderInfoVector.Back().ExitEvent = ExitEvent;
  m_CompressingCompletedEvents.Add(*m_CoderInfoVector.Back().CompressionCompletedEvent);
}

void CCoderMixer::FinishAddingCoders()
{
  for(int i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    DWORD anID;
    HANDLE aNewThread = ::CreateThread(NULL, 0, CoderThread, 
        &m_CoderInfoVector[i], 0, &anID);
    if (aNewThread == 0)
      throw 271824;
    m_Threads.Add(aNewThread);
    if (i >= 1)
    {
      CStreamBinder aStreamBinder;
      m_StreamBinders.Add(aStreamBinder);
      m_StreamBinders.Back().CreateEvents();
    }
  }
}

void CCoderMixer::ReInit()
{
  int i;
  for(i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    CThreadCoderInfo &aCoderInfo = m_CoderInfoVector[i];
    aCoderInfo.InSizeAssigned = false;
    aCoderInfo.OutSizeAssigned = false;
    aCoderInfo.Progress = NULL;
  }
  for(i = 0; i < m_StreamBinders.Size(); i++)
  {
    m_StreamBinders[i].ReInit();
  }
}

void CCoderMixer::SetCoderInfo(UINT32 coderIndex, const UINT64 *anInSize,
    const UINT64 *anOutSize)
{
  CThreadCoderInfo &aCoderInfo = m_CoderInfoVector[coderIndex];
  
  aCoderInfo.InSizeAssigned = (anInSize != NULL);
  if (aCoderInfo.InSizeAssigned)
    aCoderInfo.InSizeValue = *anInSize;

  aCoderInfo.OutSizeAssigned = (anOutSize != NULL);
  if (aCoderInfo.OutSizeAssigned)
    aCoderInfo.OutSizeValue = *anOutSize;

  aCoderInfo.Progress = NULL;
}


STDMETHODIMP CCoderMixer::Init(ISequentialInStream *anInStreamMain, 
      ISequentialOutStream *anOutStreamMain)
{
  m_CoderInfoVector.Front().InStream = anInStreamMain;
  for(int i = 0; i < m_StreamBinders.Size(); i++)
    m_StreamBinders[i].CreateStreams(&m_CoderInfoVector[i+1].InStream,
        &m_CoderInfoVector[i].OutStream);
  m_CoderInfoVector.Back().OutStream = anOutStreamMain;
  
  return S_OK;
}

bool CCoderMixer::MyCode()
{
  HANDLE anEvents[2] = { ExitEvent, m_StartCompressingEvent };
  DWORD anWaitResult = ::WaitForMultipleObjects(2, anEvents, FALSE, INFINITE);
  if (anWaitResult == WAIT_OBJECT_0 + 0)
    return false;

  for(int i = 0; i < m_CoderInfoVector.Size(); i++)
    m_CoderInfoVector[i].CompressEvent->Set();
  DWORD result = ::WaitForMultipleObjects(m_CompressingCompletedEvents.Size(), 
      &m_CompressingCompletedEvents.Front(), TRUE, INFINITE);
  
  m_CompressingFinishedEvent.Set();

  return true;
}

STDMETHODIMP CCoderMixer::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, 
    const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *progress)
{
  Init(anInStream, anOutStream);
  
  m_CompressingFinishedEvent.Reset(); // ?
  
  CComObjectNoLock<CCrossThreadProgress> *progressSpec = 
      new CComObjectNoLock<CCrossThreadProgress>;
  CComPtr<ICompressProgressInfo> aCrossProgress = progressSpec;
  progressSpec->Init();
  m_CoderInfoVector[m_ProgressCoderIndex].Progress = aCrossProgress;

  m_StartCompressingEvent.Set();

  while (true)
  {
    HANDLE anEvents[2] = {m_CompressingFinishedEvent, progressSpec->ProgressEvent };
    DWORD anWaitResult = ::WaitForMultipleObjects(2, anEvents, FALSE, INFINITE);
    if (anWaitResult == WAIT_OBJECT_0 + 0)
      break;
    if (progress != NULL)
      progressSpec->Result = progress->SetRatioInfo(progressSpec->InSize, 
          progressSpec->OutSize);
    else
      progressSpec->Result = S_OK;
    progressSpec->WaitEvent.Set();
  }

  int i;
  for(i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    HRESULT result = m_CoderInfoVector[i].Result;
    if (result == S_FALSE)
      return result;
  }
  for(i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    HRESULT result = m_CoderInfoVector[i].Result;
    if (result != S_OK)
      return result;
  }
  return S_OK;
}

UINT64 CCoderMixer::GetWriteProcessedSize(UINT32 coderIndex)
{
  return m_StreamBinders[coderIndex].ProcessedSize;
}
  
