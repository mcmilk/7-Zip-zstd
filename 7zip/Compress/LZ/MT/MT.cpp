// MT_MF.cpp

#include "StdAfx.h"

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
  // OutputDebugString("Init\n");
  m_AskChangeBufferPos.Reset();
  m_CanChangeBufferPos.Reset();
  m_BufferPosWasChanged.Reset();
  m_StopWriting.Reset();
  m_WritingWasStopped.Reset();
  m_NeedStart = true;
  HRESULT result = m_MatchFinder->Init(s);
  if (result == S_OK)
    m_DataCurrentPos = m_MatchFinder->GetPointerToCurrentPos();
  return result; 
}

STDMETHODIMP_(void) CMatchFinderMT::ReleaseStream()
{ 
  // OutputDebugString("ReleaseStream\n");
  m_StopWriting.Set();
  m_WritingWasStopped.Lock();
  // OutputDebugString("m_WritingWasStopped\n");
  m_MatchFinder->ReleaseStream(); 
}

STDMETHODIMP CMatchFinderMT::MovePos()
{ 
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
  /*

  char sz[100];
  sprintf(sz, "GetMatchLen = %d", i);
  OutputDebugString(sz);
  OutputDebugString("\n");
  */
  return i;
  // return m_MatchFinder->GetMatchLen(index, distance, limit); }
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
  delete []m_Buffer;
}

STDMETHODIMP CMatchFinderMT::Create(UInt32 sizeHistory, 
      UInt32 keepAddBufferBefore, UInt32 matchMaxLen, 
      UInt32 keepAddBufferAfter)
{ 
  FreeMem();
  m_MatchMaxLen = matchMaxLen;

  m_BlockSize = (matchMaxLen + 1) * _multiThreadMult;
  UInt32 bufferSize = m_BlockSize * kNumMTBlocks;
  m_Buffer = new UInt32[bufferSize];
  for (int i = 0; i < kNumMTBlocks; i++)
    m_Buffers[i] = &m_Buffer[i * m_BlockSize];

  m_NeedStart = true;

  keepAddBufferBefore += bufferSize;

  return m_MatchFinder->Create(sizeHistory, 
      keepAddBufferBefore, matchMaxLen, 
      keepAddBufferAfter); 
}

static DWORD WINAPI MFThread(void *threadCoderInfo)
{
  CMatchFinderMT &mt = *(CMatchFinderMT *)threadCoderInfo;
  while (true)
  {
    HANDLE events[3] = { mt.m_ExitEvent, mt.m_StopWriting, mt.m_CanWriteEvents[mt.m_WriteBufferIndex] } ;
    DWORD waitResult = ::WaitForMultipleObjects(3, events, FALSE, INFINITE);
    if (waitResult == WAIT_OBJECT_0 + 0)
      return 0;
    if (waitResult == WAIT_OBJECT_0 + 1)
    {
      // OutputDebugString("m_StopWriting\n");
      mt.m_WriteBufferIndex = 0;
      for (int i = 0; i < kNumMTBlocks; i++)
        mt.m_CanWriteEvents[i].Reset();
      mt.m_WritingWasStopped.Set();
      continue;
    }
    // OutputDebugString("m_CanWriteEvents\n");
    UInt32 *buffer = mt.m_Buffers[mt.m_WriteBufferIndex];
    UInt32 curPos = 0;
    UInt32 numBytes = 0;
    while (curPos + mt.m_MatchMaxLen + 1 <= mt.m_BlockSize)
    {
      if (mt.m_MatchFinder->GetNumAvailableBytes() == 0)
        break;
      UInt32 len = mt.m_MatchFinder->GetLongestMatch(buffer + curPos);
      /*
      if (len == 1)
        len = 0;
      */
      buffer[curPos] = len;
      curPos += len + 1;
      HRESULT result = mt.m_MatchFinder->MovePos();
      if (result != S_OK)
        throw 124459;
      numBytes++;
    }
    mt.m_LimitPos[mt.m_WriteBufferIndex] = curPos;
    mt.m_NumAvailableBytes[mt.m_WriteBufferIndex] = 
        numBytes + mt.m_MatchFinder->GetNumAvailableBytes();
    // char sz[100];
    // sprintf(sz, "x = %d", mt.m_WriteBufferIndex);
    // OutputDebugString(sz);
    // OutputDebugString("mt.m_CanReadEvents\n");
    mt.m_CanReadEvents[mt.m_WriteBufferIndex].Set();
    mt.m_WriteBufferIndex++;
    if (mt.m_WriteBufferIndex == kNumMTBlocks)
      mt.m_WriteBufferIndex = 0;
  }

  /*
  while(true)
  {
    if (!((CCoderMixer2 *)threadCoderInfo)->MyCode())
      return 0;
  }
  */
}

CMatchFinderMT::CMatchFinderMT():
  m_Buffer(0),
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
  if (HANDLE(m_Thread) != 0)
    ::WaitForSingleObject(m_Thread, INFINITE);
  FreeMem();
}

void CMatchFinderMT::Start()
{
  // OutputDebugString("Start\n");
  m_AskChangeBufferPos.Reset();
  m_CanChangeBufferPos.Reset();
  m_BufferPosWasChanged.Reset();

  m_WriteBufferIndex = 0;
  m_ReadBufferIndex = 0;
  m_NeedStart = false;
  m_CurrentPos = 0;
  m_CurrentLimitPos = 0;
  int i;
  for (i = 0; i < kNumMTBlocks; i++)
    m_CanReadEvents[i].Reset();
  for (i = kNumMTBlocks - 1; i >= 0; i--)
    m_CanWriteEvents[i].Set();
}

STDMETHODIMP_(UInt32) CMatchFinderMT::GetLongestMatch(UInt32 *distances)
{ 
  // OutputDebugString("GetLongestMatch\n");
  if (m_NeedStart)
    Start();
  /*
  if (m_CurrentPos > m_CurrentLimitPos)
    throw 1123324;
  */
  if (m_CurrentPos == m_CurrentLimitPos)
  {
    // OutputDebugString("m_CurrentPos == m_CurrentLimitPos\n");
    while (true)
    {
      /*
      char sz[100];
      sprintf(sz, "m_CanReadEvents[m_ReadBufferIndex] = %d\n", m_ReadBufferIndex);
      OutputDebugString(sz);
      OutputDebugString("\n");
      */
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
  }
  if (m_CurrentPos >= m_CurrentLimitPos)
    throw 1123324;
  const UInt32 *buffer = m_Buffers[m_ReadBufferIndex];
  UInt32 len = buffer[m_CurrentPos++];
  for (UInt32 i = 1; i <= len; i++)
    distances[i] = buffer[m_CurrentPos++];
  if (m_CurrentPos == m_CurrentLimitPos)
  {
    m_CanWriteEvents[m_ReadBufferIndex].Set();
    m_ReadBufferIndex++;
    if (m_ReadBufferIndex == kNumMTBlocks)
      m_ReadBufferIndex = 0;
  }
  // char sz[100];
  // sprintf(sz, "m_NumAvailableBytesCurrent = %d", m_NumAvailableBytesCurrent);
  // OutputDebugString(sz);
  // OutputDebugString("\n");
  return len;
}

STDMETHODIMP_(void) CMatchFinderMT::DummyLongestMatch()
{ 
  UInt32 buffer[512];
  GetLongestMatch(buffer);
  // m_MatchFinder->DummyLongestMatch();
}


