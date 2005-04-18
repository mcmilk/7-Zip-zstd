// MT.cpp

#include "StdAfx.h"

#include "../../../../Common/Alloc.h"

#include "MT.h"

class CMatchFinderCallback: 
  public IMatchFinderCallback,
  public CMyUnknownImp
{
  MY_UNKNOWN_IMP

  STDMETHOD(BeforeChangingBufferPos)();
  STDMETHOD(AfterChangingBufferPos)();
public:
  CMatchFinderMT *m_MatchFinderMT;
  const Byte *m_BufferPosBefore;
};

STDMETHODIMP CMatchFinderCallback::BeforeChangingBufferPos()
{
  m_MatchFinderMT->m_AskChangeBufferPos.Set();
  m_MatchFinderMT->m_CanChangeBufferPos.Lock();
  m_BufferPosBefore = m_MatchFinderMT->m_MatchFinder->GetPointerToCurrentPos();
  return S_OK;
}

STDMETHODIMP CMatchFinderCallback::AfterChangingBufferPos()
{
  m_MatchFinderMT->m_DataCurrentPos += 
      m_MatchFinderMT->m_MatchFinder->GetPointerToCurrentPos() - m_BufferPosBefore;
  m_MatchFinderMT->m_BufferPosWasChanged.Set();
  return S_OK;
}

HRESULT CMatchFinderMT::SetMatchFinder(IMatchFinder *matchFinder, 
    UInt32 multiThreadMult)
{
  _multiThreadMult = multiThreadMult;
  m_MatchFinder = matchFinder;
  CMyComPtr<IMatchFinderSetCallback> matchFinderSetCallback;
  if (m_MatchFinder.QueryInterface(IID_IMatchFinderSetCallback, 
      &matchFinderSetCallback) == S_OK)
  {
    CMatchFinderCallback *matchFinderCallbackSpec = 
        new CMatchFinderCallback;
    CMyComPtr<IMatchFinderCallback> matchFinderCallback = matchFinderCallbackSpec;
    matchFinderCallbackSpec->m_MatchFinderMT = this;
    matchFinderSetCallback->SetCallback(matchFinderCallback);
    return S_OK;
  }
  else
    return E_FAIL;
}


STDMETHODIMP CMatchFinderMT::Init(ISequentialInStream *s)
{ 
  m_AskChangeBufferPos.Reset();
  m_CanChangeBufferPos.Reset();
  m_BufferPosWasChanged.Reset();
  m_StopWriting.Reset();
  m_WritingWasStopped.Reset();
  m_NeedStart = true;
  m_CurrentPos = 0;
  m_CurrentLimitPos = 0;

  HRESULT result = m_MatchFinder->Init(s);
  if (result == S_OK)
    m_DataCurrentPos = m_MatchFinder->GetPointerToCurrentPos();
  return result; 
}

STDMETHODIMP_(void) CMatchFinderMT::ReleaseStream()
{ 
  m_StopWriting.Set();
  m_WritingWasStopped.Lock();
  m_MatchFinder->ReleaseStream(); 
}

STDMETHODIMP CMatchFinderMT::MovePos()
{ 
  if (m_Result != S_OK)
    return m_Result;
  m_NumAvailableBytesCurrent--;
  m_DataCurrentPos++;
  return S_OK; 
}

STDMETHODIMP_(Byte) CMatchFinderMT::GetIndexByte(Int32 index)
{ 
  return m_DataCurrentPos[index]; 
}

STDMETHODIMP_(UInt32) CMatchFinderMT::GetMatchLen(Int32 index, 
    UInt32 distance, UInt32 limit)
{ 
  if (int(index + limit) > m_NumAvailableBytesCurrent)
    limit = m_NumAvailableBytesCurrent - (index);
  distance++;
  const Byte *pby = m_DataCurrentPos + index;
  UInt32 i;
  for(i = 0; i < limit && pby[i] == pby[i - distance]; i++);
  return i;
}

STDMETHODIMP_(const Byte *) CMatchFinderMT::GetPointerToCurrentPos()
{
  return m_DataCurrentPos;
}


STDMETHODIMP_(UInt32) CMatchFinderMT::GetNumAvailableBytes()
{ 
  if (m_NeedStart)
    return m_MatchFinder->GetNumAvailableBytes(); 
  else
    return m_NumAvailableBytesCurrent;
}
  
void CMatchFinderMT::FreeMem()
{
  MyFree(m_Buffer);
  MyFree(m_DummyBuffer);
}

STDMETHODIMP CMatchFinderMT::Create(UInt32 sizeHistory, 
      UInt32 keepAddBufferBefore, UInt32 matchMaxLen, 
      UInt32 keepAddBufferAfter)
{ 
  FreeMem();
  m_MatchMaxLen = matchMaxLen;

  m_BlockSize = (matchMaxLen + 1) * _multiThreadMult;
  UInt32 bufferSize = m_BlockSize * kNumMTBlocks;
  m_DummyBuffer = (UInt32 *)MyAlloc((matchMaxLen + 1) * sizeof(UInt32));
  if (m_DummyBuffer == 0)
    return E_OUTOFMEMORY;
  m_Buffer = (UInt32 *)MyAlloc(bufferSize * sizeof(UInt32));
  if (m_Buffer == 0)
    return E_OUTOFMEMORY;
  for (int i = 0; i < kNumMTBlocks; i++)
    m_Buffers[i] = &m_Buffer[i * m_BlockSize];

  m_NeedStart = true;
  m_CurrentPos = 0;
  m_CurrentLimitPos = 0;

  keepAddBufferBefore += bufferSize;

  return m_MatchFinder->Create(sizeHistory, keepAddBufferBefore, matchMaxLen, 
      keepAddBufferAfter); 
}

static DWORD WINAPI MFThread(void *threadCoderInfo)
{
  CMatchFinderMT &mt = *(CMatchFinderMT *)threadCoderInfo;
  return mt.ThreadFunc();
}

DWORD CMatchFinderMT::ThreadFunc()
{
  bool errorMode = false;
  while (true)
  {
    HANDLE events[3] = { m_ExitEvent, m_StopWriting, m_CanWriteEvents[m_WriteBufferIndex] } ;
    DWORD waitResult = ::WaitForMultipleObjects((errorMode ? 2: 3), events, FALSE, INFINITE);
    if (waitResult == WAIT_OBJECT_0 + 0)
      return 0;
    if (waitResult == WAIT_OBJECT_0 + 1)
    {
      m_WriteBufferIndex = 0;
      for (int i = 0; i < kNumMTBlocks; i++)
        m_CanWriteEvents[i].Reset();
      m_WritingWasStopped.Set();
      errorMode = false;
      continue;
    }
    if (errorMode)
    {
      // this case means bug_in_program. So just exit;
      return 1;
    }

    m_Results[m_WriteBufferIndex] = S_OK;
    UInt32 *buffer = m_Buffers[m_WriteBufferIndex];
    UInt32 curPos = 0;
    UInt32 numBytes = 0;
    UInt32 limit = m_BlockSize - m_MatchMaxLen;
    IMatchFinder *mf = m_MatchFinder;
    do 
    {
      if (mf->GetNumAvailableBytes() == 0)
        break;
      UInt32 len = mf->GetLongestMatch(buffer + curPos);
      /*
      if (len == 1)
        len = 0;
      */
      buffer[curPos] = len;
      curPos += len + 1;
      numBytes++;
      HRESULT result = mf->MovePos();
      if (result != S_OK)
      {
        m_Results[m_WriteBufferIndex] = result;
        errorMode = true;
        break;
      }
    }
    while (curPos < limit);
    m_LimitPos[m_WriteBufferIndex] = curPos;
    if (errorMode)
      m_NumAvailableBytes[m_WriteBufferIndex] = numBytes;
    else
      m_NumAvailableBytes[m_WriteBufferIndex] = numBytes + 
          mf->GetNumAvailableBytes();
    m_CanReadEvents[m_WriteBufferIndex].Set();
    if (++m_WriteBufferIndex == kNumMTBlocks)
      m_WriteBufferIndex = 0;
  }
}

CMatchFinderMT::CMatchFinderMT():
  m_Buffer(0),
  m_DummyBuffer(0),
  _multiThreadMult(100)
{
  for (int i = 0; i < kNumMTBlocks; i++)
  {
    m_CanReadEvents[i].Reset();
    m_CanWriteEvents[i].Reset();
  }
  m_ReadBufferIndex = 0;
  m_WriteBufferIndex = 0;

  m_ExitEvent.Reset();
  if (!m_Thread.Create(MFThread, this))
    throw 271826;
}

CMatchFinderMT::~CMatchFinderMT() 
{
  m_ExitEvent.Set();
  m_Thread.Wait();
  FreeMem();
}

void CMatchFinderMT::Start()
{
  m_AskChangeBufferPos.Reset();
  m_CanChangeBufferPos.Reset();
  m_BufferPosWasChanged.Reset();

  m_WriteBufferIndex = 0;
  m_ReadBufferIndex = 0;
  m_NeedStart = false;
  m_CurrentPos = 0;
  m_CurrentLimitPos = 0;
  m_Result = S_OK;
  int i;
  for (i = 0; i < kNumMTBlocks; i++)
    m_CanReadEvents[i].Reset();
  for (i = kNumMTBlocks - 1; i >= 0; i--)
    m_CanWriteEvents[i].Set();
}

STDMETHODIMP_(UInt32) CMatchFinderMT::GetLongestMatch(UInt32 *distances)
{ 
  if (m_CurrentPos == m_CurrentLimitPos)
  {
    if (m_NeedStart)
      Start();
    while (true)
    {
      HANDLE events[2] = { m_AskChangeBufferPos, m_CanReadEvents[m_ReadBufferIndex] } ;
      DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
      if (waitResult == WAIT_OBJECT_0 + 1)
        break;
      m_BufferPosWasChanged.Reset();
      m_CanChangeBufferPos.Set();
      m_BufferPosWasChanged.Lock();
    }
    
    m_CurrentLimitPos = m_LimitPos[m_ReadBufferIndex];
    m_NumAvailableBytesCurrent = m_NumAvailableBytes[m_ReadBufferIndex];
    m_CurrentPos = 0;
    m_Result = m_Results[m_ReadBufferIndex];
  }
  /*
  if (m_CurrentPos >= m_CurrentLimitPos)
    throw 1123324;
  */
  const UInt32 *buffer = m_Buffers[m_ReadBufferIndex];
  UInt32 len = buffer[m_CurrentPos++];
  for (UInt32 i = 1; i <= len; i++)
    distances[i] = buffer[m_CurrentPos++];
  if (m_CurrentPos == m_CurrentLimitPos)
  {
    m_CanWriteEvents[m_ReadBufferIndex].Set();
    if (++m_ReadBufferIndex == kNumMTBlocks)
      m_ReadBufferIndex = 0;
  }
  return len;
}

STDMETHODIMP_(void) CMatchFinderMT::DummyLongestMatch()
{ 
  GetLongestMatch(m_DummyBuffer);
}
