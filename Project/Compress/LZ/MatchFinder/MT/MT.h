// MT_MF.h

#pragma once

#ifndef __MT_MF_H
#define __MT_MF_H

#include "Windows/Thread.h"
#include "Windows/Synchronization.h"

#include "../../../Interface/CompressInterface.h"

const kNumMTBlocks = 3;

class CMatchFinderMT: 
  public IInWindowStreamMatch,
  public CComObjectRoot
{
BEGIN_COM_MAP(CMatchFinderMT)
  COM_INTERFACE_ENTRY(IInWindowStreamMatch)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CMatchFinderMT)

  DECLARE_NO_REGISTRY()
  
  /*
  DECLARE_REGISTRY(CMatchFinderMT, TEXT("Compress.MatchFinderMT.1"), 
                 TEXT("Compress.MatchFinderMT"), 0, THREADFLAGS_APARTMENT)
  */


  STDMETHOD(Init)(ISequentialInStream *aStream);
  STDMETHOD_(void, ReleaseStream)();
  STDMETHOD(MovePos)();
  STDMETHOD_(BYTE, GetIndexByte)(UINT32 anIndex);
  STDMETHOD_(UINT32, GetMatchLen)(UINT32 aIndex, UINT32 aBack, UINT32 aLimit);
  STDMETHOD_(UINT32, GetNumAvailableBytes)();
  STDMETHOD_(const BYTE *, GetPointerToCurrentPos)();
  STDMETHOD(Create)(UINT32 aSizeHistory, 
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter);
  STDMETHOD_(UINT32, GetLongestMatch)(UINT32 *aDistances);
  STDMETHOD_(void, DummyLongestMatch)();

private:
public:
  CComPtr<IInWindowStreamMatch> m_MatchFinder;
  UINT32 m_MatchMaxLen;
  
  UINT32 m_BlockSize;
  // UINT32 m_BufferSize;
  UINT32 *m_Buffer;
  UINT32 *m_Buffers[kNumMTBlocks];

  bool m_NeedStart;
  UINT32 m_WriteBufferIndex;
  UINT32 m_ReadBufferIndex;

  NWindows::NSynchronization::CAutoResetEvent m_StopWriting;
  NWindows::NSynchronization::CAutoResetEvent m_WritingWasStopped;

  NWindows::NSynchronization::CAutoResetEvent m_AskChangeBufferPos;
  NWindows::NSynchronization::CAutoResetEvent m_CanChangeBufferPos;
  NWindows::NSynchronization::CAutoResetEvent m_BufferPosWasChanged;

  NWindows::NSynchronization::CManualResetEvent m_ExitEvent;
  // NWindows::NSynchronization::CManualResetEvent m_NewStart;
  NWindows::NSynchronization::CAutoResetEvent m_CanReadEvents[kNumMTBlocks];
  NWindows::NSynchronization::CAutoResetEvent m_CanWriteEvents[kNumMTBlocks];
  UINT32 m_LimitPos[kNumMTBlocks];
  UINT32 m_NumAvailableBytes[kNumMTBlocks];

  UINT32 m_NumAvailableBytesCurrent;
  const BYTE *m_DataCurrentPos;
  
  UINT32 m_CurrentLimitPos;
  UINT32 m_CurrentPos;

  NWindows::CThread m_Thread;
  // bool m_WriteWasClosed;
  UINT32 _multiThreadMult;
public:
  CMatchFinderMT();
  ~CMatchFinderMT();
  void Start();
  void FreeMem();
  HRESULT SetMatchFinder(IInWindowStreamMatch *aMatchFinder, 
      UINT32 multiThreadMult);
};
 

#endif

