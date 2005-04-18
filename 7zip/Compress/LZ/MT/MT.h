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

  UInt32 m_CurrentPos;
  UInt32 m_CurrentLimitPos;
  UInt32 m_MatchMaxLen;
  
  UInt32 m_BlockSize;
  UInt32 *m_Buffer;
  UInt32 *m_Buffers[kNumMTBlocks];
  UInt32 *m_DummyBuffer;

  bool m_NeedStart;
  UInt32 m_WriteBufferIndex;
  UInt32 m_ReadBufferIndex;

  NWindows::NSynchronization::CAutoResetEvent m_StopWriting;
  NWindows::NSynchronization::CAutoResetEvent m_WritingWasStopped;

  NWindows::NSynchronization::CManualResetEvent m_ExitEvent;
  NWindows::NSynchronization::CAutoResetEvent m_CanReadEvents[kNumMTBlocks];
  NWindows::NSynchronization::CAutoResetEvent m_CanWriteEvents[kNumMTBlocks];
  HRESULT m_Results[kNumMTBlocks];

  UInt32 m_LimitPos[kNumMTBlocks];
  UInt32 m_NumAvailableBytes[kNumMTBlocks];

  UInt32 m_NumAvailableBytesCurrent;
  
  NWindows::CThread m_Thread;
  UInt32 _multiThreadMult;

  HRESULT m_Result;

  void Start();
  void FreeMem();

public:
  NWindows::NSynchronization::CAutoResetEvent m_AskChangeBufferPos;
  NWindows::NSynchronization::CAutoResetEvent m_CanChangeBufferPos;
  NWindows::NSynchronization::CAutoResetEvent m_BufferPosWasChanged;
  CMyComPtr<IMatchFinder> m_MatchFinder;
  const Byte *m_DataCurrentPos;

  DWORD ThreadFunc();
  
  CMatchFinderMT();
  ~CMatchFinderMT();
  HRESULT SetMatchFinder(IMatchFinder *matchFinder, UInt32 multiThreadMult = 200);
};
 
#endif
