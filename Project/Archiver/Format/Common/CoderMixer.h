// CoderMixer.h

#pragma once

#ifndef __CODERMIXER_H
#define __CODERMIXER_H

#include "Util/StreamBinder.h"
#include "../../../Compress/Interface/CompressInterface.h"
#include "Common/Vector.h"
#include "Windows/Thread.h"

//////////////////////////
// CCoderHolder

struct CThreadCoderInfo
{
  NWindows::NSynchronization::CAutoResetEvent *CompressEvent;
  HANDLE ExitEvent;
  NWindows::NSynchronization::CAutoResetEvent *CompressionCompletedEvent;

  CComPtr<ICompressCoder> Coder;
  CComPtr<ISequentialInStream> InStream;
  CComPtr<ISequentialOutStream> OutStream;
  UINT64 InSizeValue;
  UINT64 OutSizeValue;
  bool InSizeAssigned;
  bool OutSizeAssigned;
  ICompressProgressInfo *Progress;
  HRESULT Result;

  CThreadCoderInfo(): ExitEvent(NULL), CompressEvent(NULL), 
      CompressionCompletedEvent(NULL), 
      InSizeAssigned(false), 
      OutSizeAssigned(false), Progress(NULL){}
  ~CThreadCoderInfo();
  bool WaitAndCode();
  void CreateEvents();
};


class CCoderMixer:
  public ICompressCoder,
  public CComObjectRoot
{
BEGIN_COM_MAP(CCoderMixer)
  COM_INTERFACE_ENTRY(ICompressCoder)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCoderMixer)

DECLARE_NO_REGISTRY()

public:
  STDMETHOD(Init)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream);
  // STDMETHOD(ReleaseStreams)();
  // STDMETHOD(Code)(UINT32 aSize, UINT32 &aProcessedSize);
  // STDMETHOD(Flush)();

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);


  CCoderMixer();
  ~CCoderMixer();
  void AddCoder(ICompressCoder *aCoder);
  void FinishAddingCoders();

  void ReInit();
  void SetCoderInfo(UINT32 aCoderIndex, 
      const UINT64 *anInSize, const UINT64 *anOutSize);
  void SetProgressCoderIndex(UINT32 aCoderIndex)
    {  m_ProgressCoderIndex = aCoderIndex; }
  UINT64 GetWriteProcessedSize(UINT32 aCoderIndex);

  bool MyCode();
private:
  CObjectVector<CStreamBinder> m_StreamBinders;
  CObjectVector<CThreadCoderInfo> m_CoderInfoVector;
  CRecordVector<HANDLE> m_Threads;
  NWindows::CThread m_MainThread;

  NWindows::NSynchronization::CAutoResetEvent m_StartCompressingEvent;
  CRecordVector<HANDLE> m_CompressingCompletedEvents;
  NWindows::NSynchronization::CAutoResetEvent m_CompressingFinishedEvent;

  NWindows::NSynchronization::CManualResetEvent ExitEvent;
  UINT32 m_ProgressCoderIndex;
};

#endif

