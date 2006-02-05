// MT.cpp

#include "StdAfx.h"

#include "../../../../Common/Alloc.h"

#include "MT.h"

static const UInt32 kBlockSize = (1 << 14);

static DWORD WINAPI MFThread(void *threadCoderInfo)
{
  return ((CMatchFinderMT *)threadCoderInfo)->ThreadFunc();
}

CMatchFinderMT::CMatchFinderMT():
  m_Buffer(0),
  m_NeedStart(true)
{
  m_BlockIndex = kNumMTBlocks - 1;
  m_CS[m_BlockIndex].Enter();
  if (!m_Thread.Create(MFThread, this))
    throw 271826;
}

CMatchFinderMT::~CMatchFinderMT() 
{
  m_Exit = true;
  m_CS[m_BlockIndex].Leave();
  m_CanChangeBufferPos.Set();
  if (m_NeedStart)
    m_MtCanStart.Set();
  m_Thread.Wait();
  FreeMem();
}

void CMatchFinderMT::FreeMem()
{
  ::MyFree(m_Buffer);
  m_Buffer = 0;
}

STDMETHODIMP CMatchFinderMT::Create(UInt32 sizeHistory, UInt32 keepAddBufferBefore, 
    UInt32 matchMaxLen, UInt32 keepAddBufferAfter)
{ 
  FreeMem();
  m_MatchMaxLen = matchMaxLen;
  if (kBlockSize <= matchMaxLen * 4)
    return E_INVALIDARG;
  UInt32 bufferSize = kBlockSize * kNumMTBlocks;
  m_Buffer = (UInt32 *)::MyAlloc(bufferSize * sizeof(UInt32));
  if (m_Buffer == 0)
    return E_OUTOFMEMORY;
  keepAddBufferBefore += bufferSize;
  keepAddBufferAfter += (kBlockSize + 1);
  return m_MatchFinder->Create(sizeHistory, keepAddBufferBefore, matchMaxLen, keepAddBufferAfter); 
}

// UInt32 blockSizeMult = 800
HRESULT CMatchFinderMT::SetMatchFinder(IMatchFinder *matchFinder)
{
  m_MatchFinder = matchFinder;
  return S_OK;
}

STDMETHODIMP CMatchFinderMT::SetStream(ISequentialInStream *s)
{ 
  return m_MatchFinder->SetStream(s);
}

// Call it after ReleaseStream / SetStream
STDMETHODIMP CMatchFinderMT::Init()
{ 
  m_NeedStart = true;
  m_Pos = 0;
  m_PosLimit = 0;

  HRESULT result = m_MatchFinder->Init();
  if (result == S_OK)
    m_DataCurrentPos = m_MatchFinder->GetPointerToCurrentPos();
  m_NumAvailableBytes = m_MatchFinder->GetNumAvailableBytes();
  return result; 
}

// ReleaseStream is required to finish multithreading
STDMETHODIMP_(void) CMatchFinderMT::ReleaseStream()
{ 
  m_StopWriting = true;
  m_CS[m_BlockIndex].Leave();
  if (!m_NeedStart)
  {
    m_CanChangeBufferPos.Set();
    m_MtWasStopped.Lock();
    m_NeedStart = true;
  }
  m_MatchFinder->ReleaseStream();
  m_BlockIndex = kNumMTBlocks - 1;
  m_CS[m_BlockIndex].Enter();
}

STDMETHODIMP_(Byte) CMatchFinderMT::GetIndexByte(Int32 index)
{ 
  return m_DataCurrentPos[index]; 
}

STDMETHODIMP_(UInt32) CMatchFinderMT::GetMatchLen(Int32 index, UInt32 distance, UInt32 limit)
{ 
  if ((Int32)(index + limit) > m_NumAvailableBytes)
    limit = m_NumAvailableBytes - (index);
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
  return m_NumAvailableBytes;
}
  
void CMatchFinderMT::GetNextBlock()
{
  if (m_NeedStart)
  {
    m_NeedStart = false;
    for (UInt32 i = 0; i < kNumMTBlocks; i++)
      m_StopReading[i] = false;
    m_StopWriting = false;
    m_Exit = false;
    m_MtWasStarted.Reset();
    m_MtWasStopped.Reset();
    m_CanChangeBufferPos.Reset();
    m_BufferPosWasChanged.Reset();
    m_MtCanStart.Set();
    m_MtWasStarted.Lock();
    m_Result = S_OK;
  }
  while (true)
  {
    UInt32 nextIndex = (m_BlockIndex == kNumMTBlocks - 1) ? 0 : m_BlockIndex + 1;
    m_CS[nextIndex].Enter();
    if (!m_StopReading[nextIndex])
    {
      m_CS[m_BlockIndex].Leave();
      m_BlockIndex = nextIndex;
      break;
    }
    m_StopReading[nextIndex] = false;
    m_CS[nextIndex].Leave();
    m_CanChangeBufferPos.Set();
    m_BufferPosWasChanged.Lock();
    m_CS[nextIndex].Enter();
    m_CS[m_BlockIndex].Leave();
    m_BlockIndex = nextIndex;
  }
  m_Pos = m_BlockIndex * kBlockSize;
  m_PosLimit = m_Buffer[m_Pos++];
  m_NumAvailableBytes = m_Buffer[m_Pos++];
  m_Result = m_Results[m_BlockIndex];
}

STDMETHODIMP CMatchFinderMT::GetMatches(UInt32 *distances)
{ 
  if (m_Pos == m_PosLimit)
    GetNextBlock();

  if (m_Result != S_OK)
    return m_Result;
  m_NumAvailableBytes--;
  m_DataCurrentPos++;

  const UInt32 *buffer = m_Buffer + m_Pos;
  UInt32 len = *buffer++;
  *distances++ = len;
  m_Pos += 1 + len;
  for (UInt32 i = 0; i != len; i += 2)
  {
    distances[i] = buffer[i];
    distances[i + 1] = buffer[i + 1];
  }
  return S_OK; 
}

STDMETHODIMP CMatchFinderMT::Skip(UInt32 num)
{ 
  do
  {
    if (m_Pos == m_PosLimit)
      GetNextBlock();
    
    if (m_Result != S_OK)
      return m_Result;
    m_NumAvailableBytes--;
    m_DataCurrentPos++;
    
    UInt32 len = m_Buffer[m_Pos++];
    m_Pos += len;
  }
  while(--num != 0);
  return S_OK; 
}

STDMETHODIMP_(Int32) CMatchFinderMT::NeedChangeBufferPos(UInt32 numCheckBytes)
{
  throw 1;
}

STDMETHODIMP_(void) CMatchFinderMT::ChangeBufferPos()
{
  throw 1;
}


DWORD CMatchFinderMT::ThreadFunc()
{
  while(true)
  {
    bool needStartEvent = true;
    m_MtCanStart.Lock();
    HRESULT result = S_OK;
    UInt32 blockIndex = 0;
    while (true)
    {
      m_CS[blockIndex].Enter();
      if (needStartEvent)
      {
        m_MtWasStarted.Set();
        needStartEvent = false;
      }
      else
        m_CS[(blockIndex == 0) ? kNumMTBlocks - 1 : blockIndex - 1].Leave();
      if (m_Exit)
        return 0;
      if (m_StopWriting)
      {
        m_MtWasStopped.Set();
        m_CS[blockIndex].Leave();
        break;
      }
      if (result == S_OK)
      {
        IMatchFinder *mf = m_MatchFinder;
        if (mf->NeedChangeBufferPos(kBlockSize) != 0)
        {
          // m_AskChangeBufferPos.Set();
          m_StopReading[blockIndex] = true;
          m_CS[blockIndex].Leave();
          m_CanChangeBufferPos.Lock();
          m_CS[blockIndex].Enter();
          const Byte *bufferPosBefore = mf->GetPointerToCurrentPos();
          mf->ChangeBufferPos();
          m_DataCurrentPos += mf->GetPointerToCurrentPos() - bufferPosBefore;
          m_BufferPosWasChanged.Set();
        }
        else
        {
          UInt32 curPos = blockIndex * kBlockSize;
          UInt32 limit = curPos + kBlockSize - m_MatchMaxLen - m_MatchMaxLen - 1;
          UInt32 *buffer = m_Buffer;
          m_Results[blockIndex] = S_OK;
          curPos++;
          UInt32 numAvailableBytes = mf->GetNumAvailableBytes();
          buffer[curPos++] = numAvailableBytes;
          
          while (numAvailableBytes-- != 0 && curPos < limit)
          {
            result = mf->GetMatches(buffer + curPos);
            if (result != S_OK)
            {
              m_Results[blockIndex] = result;
              break;
            }
            curPos += buffer[curPos] + 1;
          }
          buffer[blockIndex * kBlockSize] = curPos;
        }
      }
      else
      {
        UInt32 curPos = blockIndex * kBlockSize;
        m_Buffer[curPos] = curPos + 2; // size of buffer
        m_Buffer[curPos + 1] = 0;      // NumAvailableBytes
        m_Results[blockIndex] = result; // error
      }
      if (++blockIndex == kNumMTBlocks)
        blockIndex = 0;
    }
  }
}
