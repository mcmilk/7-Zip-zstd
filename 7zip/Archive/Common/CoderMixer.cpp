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
  CCoderInfoFlusher(CThreadCoderInfo *coderInfo): m_CoderInfo(coderInfo) {}
  ~CCoderInfoFlusher()
  {
    m_CoderInfo->InStream.Release();
    m_CoderInfo->OutStream.Release();
    m_CoderInfo->CompressionCompletedEvent->Set();
  }
};

bool CThreadCoderInfo::WaitAndCode()
{
  HANDLE events[2] = { ExitEvent, *CompressEvent };
  DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
  if (waitResult == WAIT_OBJECT_0 + 0)
    return false;

  {
    CCoderInfoFlusher coderInfoFlusher(this);
    Result = Coder->Code(InStream, OutStream, 
        InSizeAssigned ? &InSizeValue : NULL, 
        OutSizeAssigned ? &OutSizeValue : NULL, 
        Progress);
  }
  return true;
}


DWORD WINAPI CoderThread(void *threadCoderInfo)
{
  while(true)
  {
    if (!((CThreadCoderInfo *)threadCoderInfo)->WaitAndCode())
      return 0;
  }
}

//////////////////////////
// CCoderMixer

static DWORD WINAPI MainCoderThread(void *threadCoderInfo)
{
  while(true)
  {
    if (!((CCoderMixer *)threadCoderInfo)->MyCode())
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
      &m_Threads.Front(), TRUE, INFINITE);
  for(int i = 0; i < m_Threads.Size(); i++)
    ::CloseHandle(m_Threads[i]);
}


void CCoderMixer::AddCoder(ICompressCoder *coder)
{
  CThreadCoderInfo threadCoderInfo;
  threadCoderInfo.Coder = coder;
  m_CoderInfoVector.Add(threadCoderInfo);
  m_CoderInfoVector.Back().CreateEvents();
  m_CoderInfoVector.Back().ExitEvent = ExitEvent;
  m_CompressingCompletedEvents.Add(*m_CoderInfoVector.Back().CompressionCompletedEvent);
}

void CCoderMixer::FinishAddingCoders()
{
  for(int i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    DWORD id;
    HANDLE newThread = ::CreateThread(NULL, 0, CoderThread, 
        &m_CoderInfoVector[i], 0, &id);
    if (newThread == 0)
      throw 271824;
    m_Threads.Add(newThread);
    if (i >= 1)
    {
      CStreamBinder streamBinder;
      m_StreamBinders.Add(streamBinder);
      m_StreamBinders.Back().CreateEvents();
    }
  }
}

void CCoderMixer::ReInit()
{
  int i;
  for(i = 0; i < m_CoderInfoVector.Size(); i++)
  {
    CThreadCoderInfo &coderInfo = m_CoderInfoVector[i];
    coderInfo.InSizeAssigned = false;
    coderInfo.OutSizeAssigned = false;
    coderInfo.Progress = NULL;
  }
  for(i = 0; i < m_StreamBinders.Size(); i++)
  {
    m_StreamBinders[i].ReInit();
  }
}

void CCoderMixer::SetCoderInfo(UInt32 coderIndex, const UInt64 *inSize,
    const UInt64 *outSize)
{
  CThreadCoderInfo &coderInfo = m_CoderInfoVector[coderIndex];
  
  coderInfo.InSizeAssigned = (inSize != NULL);
  if (coderInfo.InSizeAssigned)
    coderInfo.InSizeValue = *inSize;

  coderInfo.OutSizeAssigned = (outSize != NULL);
  if (coderInfo.OutSizeAssigned)
    coderInfo.OutSizeValue = *outSize;

  coderInfo.Progress = NULL;
}

STDMETHODIMP CCoderMixer::Init(ISequentialInStream *inStreamMain, 
      ISequentialOutStream *outStreamMain)
{
  m_CoderInfoVector.Front().InStream = inStreamMain;
  for(int i = 0; i < m_StreamBinders.Size(); i++)
    m_StreamBinders[i].CreateStreams(&m_CoderInfoVector[i+1].InStream,
        &m_CoderInfoVector[i].OutStream);
  m_CoderInfoVector.Back().OutStream = outStreamMain;
  
  return S_OK;
}

bool CCoderMixer::MyCode()
{
  HANDLE events[2] = { ExitEvent, m_StartCompressingEvent };
  DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
  if (waitResult == WAIT_OBJECT_0 + 0)
    return false;

  for(int i = 0; i < m_CoderInfoVector.Size(); i++)
    m_CoderInfoVector[i].CompressEvent->Set();
  DWORD result = ::WaitForMultipleObjects(m_CompressingCompletedEvents.Size(), 
      &m_CompressingCompletedEvents.Front(), TRUE, INFINITE);
  
  m_CompressingFinishedEvent.Set();

  return true;
}

STDMETHODIMP CCoderMixer::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, 
    const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  Init(inStream, outStream);
  
  m_CompressingFinishedEvent.Reset(); // ?
  
  CCrossThreadProgress *progressSpec = new CCrossThreadProgress;
  CMyComPtr<ICompressProgressInfo> crossProgress = progressSpec;
  progressSpec->Init();
  m_CoderInfoVector[m_ProgressCoderIndex].Progress = crossProgress;

  m_StartCompressingEvent.Set();

  while (true)
  {
    HANDLE events[2] = {m_CompressingFinishedEvent, progressSpec->ProgressEvent };
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

UInt64 CCoderMixer::GetWriteProcessedSize(UInt32 coderIndex)
{
  return m_StreamBinders[coderIndex].ProcessedSize;
}
  
