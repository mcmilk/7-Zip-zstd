// MT_MF.cpp

#include "StdAfx.h"

#include "MT.h"

class CMatchFinderCallback: 
  public IMatchFinderCallback,
  public CComObjectRoot
{
BEGIN_COM_MAP(CMatchFinderCallback)
  COM_INTERFACE_ENTRY(IMatchFinderCallback)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CMatchFinderCallback)

DECLARE_NO_REGISTRY()

  STDMETHOD(BeforeChangingBufferPos)();
  STDMETHOD(AfterChangingBufferPos)();
public:
  CMatchFinderMT *m_MatchFinderMT;
  const BYTE *m_BufferPosBefore;
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

HRESULT CMatchFinderMT::SetMatchFinder(IInWindowStreamMatch *aMatchFinder)
{
  m_MatchFinder = aMatchFinder;
  CComPtr<IMatchFinderSetCallback> aMatchFinderSetCallback;
  if (m_MatchFinder.QueryInterface(&aMatchFinderSetCallback) == S_OK)
  {
    CMatchFinderCallback *aMatchFinderCallbackSpec = 
        new CComObjectNoLock<CMatchFinderCallback>;
    CComPtr<IMatchFinderCallback> aMatchFinderCallback = aMatchFinderCallbackSpec;
    aMatchFinderCallbackSpec->m_MatchFinderMT = this;
    aMatchFinderSetCallback->SetCallback(aMatchFinderCallback);
    return S_OK;
  }
  else
    return E_FAIL;
}


STDMETHODIMP CMatchFinderMT::Init(ISequentialInStream *aStream)
{ 
  // OutputDebugString("Init\n");
  m_AskChangeBufferPos.Reset();
  m_CanChangeBufferPos.Reset();
  m_BufferPosWasChanged.Reset();
  m_StopWriting.Reset();
  m_WritingWasStopped.Reset();
  m_NeedStart = true;
  HRESULT aResult = m_MatchFinder->Init(aStream);
  if (aResult == S_OK)
    m_DataCurrentPos = m_MatchFinder->GetPointerToCurrentPos();
  return aResult; 
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

STDMETHODIMP_(BYTE) CMatchFinderMT::GetIndexByte(UINT32 anIndex)
{ 
  return m_DataCurrentPos[anIndex]; 
}

STDMETHODIMP_(UINT32) CMatchFinderMT::GetMatchLen(UINT32 aIndex, 
    UINT32 aBack, UINT32 aLimit)
{ 
  if (int(aIndex + aLimit) > m_NumAvailableBytesCurrent)
    aLimit = m_NumAvailableBytesCurrent - (aIndex);
  aBack++;
  const BYTE *pby = m_DataCurrentPos + aIndex;
  for(UINT32 i = 0; i < aLimit && pby[i] == pby[i - aBack]; i++);
  /*

  char aSz[100];
  sprintf(aSz, "GetMatchLen = %d", i);
  OutputDebugString(aSz);
  OutputDebugString("\n");
  */
  return i;
  // return m_MatchFinder->GetMatchLen(aIndex, aBack, aLimit); }
}

STDMETHODIMP_(const BYTE *) CMatchFinderMT::GetPointerToCurrentPos()
{
  return m_DataCurrentPos;
}


STDMETHODIMP_(UINT32) CMatchFinderMT::GetNumAvailableBytes()
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

STDMETHODIMP CMatchFinderMT::Create(UINT32 aSizeHistory, 
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter)
{ 
  FreeMem();
  m_MatchMaxLen = aMatchMaxLen;

  m_BlockSize = (aMatchMaxLen + 1) * m_MultiThreadMult;
  UINT32 aBufferSize = m_BlockSize * kNumMTBlocks;
  m_Buffer = new UINT32[aBufferSize];
  for (int i = 0; i < kNumMTBlocks; i++)
    m_Buffers[i] = &m_Buffer[i * m_BlockSize];

  m_NeedStart = true;

  aKeepAddBufferBefore += aBufferSize;

  return m_MatchFinder->Create(aSizeHistory, 
      aKeepAddBufferBefore, aMatchMaxLen, 
      aKeepAddBufferAfter); 
}

static DWORD WINAPI MFThread(void *aThreadCoderInfo)
{
  CMatchFinderMT &aMT = *(CMatchFinderMT *)aThreadCoderInfo;
  while (true)
  {
    HANDLE anEvents[3] = { aMT.m_ExitEvent, aMT.m_StopWriting, aMT.m_CanWriteEvents[aMT.m_WriteBufferIndex] } ;
    DWORD anWaitResult = ::WaitForMultipleObjects(3, anEvents, FALSE, INFINITE);
    if (anWaitResult == WAIT_OBJECT_0 + 0)
      return 0;
    if (anWaitResult == WAIT_OBJECT_0 + 1)
    {
      // OutputDebugString("m_StopWriting\n");
      aMT.m_WriteBufferIndex = 0;
      for (int i = 0; i < kNumMTBlocks; i++)
        aMT.m_CanWriteEvents[i].Reset();
      aMT.m_WritingWasStopped.Set();
      continue;
    }
    // OutputDebugString("m_CanWriteEvents\n");
    UINT32 *aBuffer = aMT.m_Buffers[aMT.m_WriteBufferIndex];
    UINT32 aCurPos = 0;
    UINT32 aNumBytes = 0;
    while (aCurPos + aMT.m_MatchMaxLen + 1 <= aMT.m_BlockSize)
    {
      if (aMT.m_MatchFinder->GetNumAvailableBytes() == 0)
        break;
      UINT32 aLen = aMT.m_MatchFinder->GetLongestMatch(aBuffer + aCurPos);
      /*
      if (aLen == 1)
        aLen = 0;
      */
      aBuffer[aCurPos] = aLen;
      aCurPos += aLen + 1;
      HRESULT aResult = aMT.m_MatchFinder->MovePos();
      if (aResult != S_OK)
        throw 124459;
      aNumBytes++;
    }
    aMT.m_LimitPos[aMT.m_WriteBufferIndex] = aCurPos;
    aMT.m_NumAvailableBytes[aMT.m_WriteBufferIndex] = 
        aNumBytes + aMT.m_MatchFinder->GetNumAvailableBytes();
    // char aSz[100];
    // sprintf(aSz, "x = %d", aMT.m_WriteBufferIndex);
    // OutputDebugString(aSz);
    // OutputDebugString("aMT.m_CanReadEvents\n");
    aMT.m_CanReadEvents[aMT.m_WriteBufferIndex].Set();
    aMT.m_WriteBufferIndex++;
    if (aMT.m_WriteBufferIndex == kNumMTBlocks)
      aMT.m_WriteBufferIndex = 0;
  }

  /*
  while(true)
  {
    if (!((CCoderMixer2 *)aThreadCoderInfo)->MyCode())
      return 0;
  }
  */
}

CMatchFinderMT::CMatchFinderMT():
  m_Buffer(0),
  m_MultiThreadMult(100)
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
  for (int i = 0; i < kNumMTBlocks; i++)
    m_CanReadEvents[i].Reset();
  for (i = kNumMTBlocks - 1; i >= 0; i--)
    m_CanWriteEvents[i].Set();
}

STDMETHODIMP_(UINT32) CMatchFinderMT::GetLongestMatch(UINT32 *aDistances)
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
      char aSz[100];
      sprintf(aSz, "m_CanReadEvents[m_ReadBufferIndex] = %d\n", m_ReadBufferIndex);
      OutputDebugString(aSz);
      OutputDebugString("\n");
      */
      HANDLE anEvents[2] = { m_AskChangeBufferPos, m_CanReadEvents[m_ReadBufferIndex] } ;
      DWORD anWaitResult = ::WaitForMultipleObjects(2, anEvents, FALSE, INFINITE);
      if (anWaitResult == WAIT_OBJECT_0 + 1)
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
  const UINT32 *aBuffer = m_Buffers[m_ReadBufferIndex];
  UINT32 aLen = aBuffer[m_CurrentPos++];
  for (UINT32 i = 1; i <= aLen; i++)
    aDistances[i] = aBuffer[m_CurrentPos++];
  if (m_CurrentPos == m_CurrentLimitPos)
  {
    m_CanWriteEvents[m_ReadBufferIndex].Set();
    m_ReadBufferIndex++;
    if (m_ReadBufferIndex == kNumMTBlocks)
      m_ReadBufferIndex = 0;
  }
  // char aSz[100];
  // sprintf(aSz, "m_NumAvailableBytesCurrent = %d", m_NumAvailableBytesCurrent);
  // OutputDebugString(aSz);
  // OutputDebugString("\n");
  return aLen;
}

STDMETHODIMP_(void) CMatchFinderMT::DummyLongestMatch()
{ 
  UINT32 aBuffer[512];
  GetLongestMatch(aBuffer);
  // m_MatchFinder->DummyLongestMatch();
}


