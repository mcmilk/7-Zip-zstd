// LZ/MT.h

#ifndef __LZ_MT_H
#define __LZ_MT_H

#include "../../../../Common/MyCom.h"

#include "../../../../Windows/Thread.h"
#include "../../../../Windows/Synchronization.h"

#include "../../../ICoder.h"
#include "../IMatchFinder.h"

const UInt32 kNumMTBlocks = (1 << 6);

class CMatchFinderMT: 
  public IMatchFinder,
  public CMyUnknownImp
{
  MY_UNKNOWN_IMP

  STDMETHOD(SetStream)(ISequentialInStream *inStream);
  STDMETHOD_(void, ReleaseStream)();
  STDMETHOD(Init)();
  STDMETHOD_(Byte, GetIndexByte)(Int32 index);
  STDMETHOD_(UInt32, GetMatchLen)(Int32 index, UInt32 distance, UInt32 limit);
  STDMETHOD_(UInt32, GetNumAvailableBytes)();
  STDMETHOD_(const Byte *, GetPointerToCurrentPos)();
  STDMETHOD(Create)(UInt32 sizeHistory, UInt32 keepAddBufferBefore, 
      UInt32 matchMaxLen, UInt32 keepAddBufferAfter);
  STDMETHOD(GetMatches)(UInt32 *distances);
  STDMETHOD(Skip)(UInt32 num);

  STDMETHOD_(Int32, NeedChangeBufferPos)(UInt32 numCheckBytes);
  STDMETHOD_(void, ChangeBufferPos)();

  UInt32 m_Pos;
  UInt32 m_PosLimit;
  UInt32 m_MatchMaxLen;
  
  UInt32 *m_Buffer;

  bool m_NeedStart;
  UInt32 m_BlockIndex;
  HRESULT m_Result;
  UInt32 m_NumAvailableBytes;
  const Byte *m_DataCurrentPos;

  // Common variables

  CMyComPtr<IMatchFinder> m_MatchFinder;
  NWindows::CThread m_Thread;
  NWindows::NSynchronization::CAutoResetEvent m_MtCanStart;
  NWindows::NSynchronization::CAutoResetEvent m_MtWasStarted;
  NWindows::NSynchronization::CAutoResetEvent m_MtWasStopped;
  NWindows::NSynchronization::CAutoResetEvent m_CanChangeBufferPos;
  NWindows::NSynchronization::CAutoResetEvent m_BufferPosWasChanged;

  NWindows::NSynchronization::CCriticalSection m_CS[kNumMTBlocks];

  HRESULT m_Results[kNumMTBlocks];
  bool m_StopReading[kNumMTBlocks];
  bool m_Exit;
  bool m_StopWriting;

  ////////////////////////////

  void FreeMem();
  void GetNextBlock();
public:

  DWORD ThreadFunc();
  
  CMatchFinderMT();
  ~CMatchFinderMT();
  HRESULT SetMatchFinder(IMatchFinder *matchFinder);
};
 
#endif
