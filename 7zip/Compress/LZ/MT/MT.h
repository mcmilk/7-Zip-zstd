// LZ/MT.h

#ifndef __LZ_MT_H
#define __LZ_MT_H

#include "../../../../Common/MyCom.h"

#include "../../../../Windows/Thread.h"
#include "../../../../Windows/Synchronization.h"

#include "../../../ICoder.h"
#include "../IMatchFinder.h"

const int kNumMTBlocks = 3;

class CMatchFinderMT: 
  public IMatchFinder,
  public CMyUnknownImp
{
  MY_UNKNOWN_IMP

  STDMETHOD(Init)(ISequentialInStream *s);
  STDMETHOD_(void, ReleaseStream)();
  STDMETHOD(MovePos)();
  STDMETHOD_(Byte, GetIndexByte)(Int32 index);
  STDMETHOD_(UInt32, GetMatchLen)(Int32 index, UInt32 distance, UInt32 limit);
  STDMETHOD_(UInt32, GetNumAvailableBytes)();
  STDMETHOD_(const Byte *, GetPointerToCurrentPos)();
  STDMETHOD(Create)(UInt32 sizeHistory, 
      UInt32 keepAddBufferBefore, UInt32 matchMaxLen, 
      UInt32 keepAddBufferAfter);
  STDMETHOD_(UInt32, GetLongestMatch)(UInt32 *distances);
  STDMETHOD_(void, DummyLongestMatch)();

private:
public:
  CMyComPtr<IMatchFinder> m_MatchFinder;
  UInt32 m_MatchMaxLen;
  
  UInt32 m_BlockSize;
  // UInt32 m_BufferSize;
  UInt32 *m_Buffer;
  UInt32 *m_Buffers[kNumMTBlocks];

  bool m_NeedStart;
  UInt32 m_WriteBufferIndex;
  UInt32 m_ReadBufferIndex;

  NWindows::NSynchronization::CAutoResetEvent m_StopWriting;
  NWindows::NSynchronization::CAutoResetEvent m_WritingWasStopped;

  NWindows::NSynchronization::CAutoResetEvent m_AskChangeBufferPos;
  NWindows::NSynchronization::CAutoResetEvent m_CanChangeBufferPos;
  NWindows::NSynchronization::CAutoResetEvent m_BufferPosWasChanged;

  NWindows::NSynchronization::CManualResetEvent m_ExitEvent;
  // NWindows::NSynchronization::CManualResetEvent m_NewStart;
  NWindows::NSynchronization::CAutoResetEvent m_CanReadEvents[kNumMTBlocks];
  NWindows::NSynchronization::CAutoResetEvent m_CanWriteEvents[kNumMTBlocks];
  UInt32 m_LimitPos[kNumMTBlocks];
  UInt32 m_NumAvailableBytes[kNumMTBlocks];

  UInt32 m_NumAvailableBytesCurrent;
  const Byte *m_DataCurrentPos;
  
  UInt32 m_CurrentLimitPos;
  UInt32 m_CurrentPos;

  NWindows::CThread m_Thread;
  // bool m_WriteWasClosed;
  UInt32 _multiThreadMult;
public:
  CMatchFinderMT();
  ~CMatchFinderMT();
  void Start();
  void FreeMem();
  HRESULT SetMatchFinder(IMatchFinder *matchFinder, 
      UInt32 multiThreadMult = 200);
};
 

#endif

