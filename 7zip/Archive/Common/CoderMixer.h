// CoderMixer.h

#ifndef __CODERMIXER_H
#define __CODERMIXER_H

#include "Common/Vector.h"
#include "Common/MyCom.h"
#include "Windows/Thread.h"
#include "../../Common/StreamBinder.h"
#include "../../ICoder.h"

struct CThreadCoderInfo
{
  NWindows::NSynchronization::CAutoResetEvent *CompressEvent;
  HANDLE ExitEvent;
  NWindows::NSynchronization::CAutoResetEvent *CompressionCompletedEvent;

  CMyComPtr<ICompressCoder> Coder;
  CMyComPtr<ISequentialInStream> InStream;
  CMyComPtr<ISequentialOutStream> OutStream;
  UInt64 InSizeValue;
  UInt64 OutSizeValue;
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
  public CMyUnknownImp
{
  MY_UNKNOWN_IMP

public:
  STDMETHOD(Init)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream);
  // STDMETHOD(ReleaseStreams)();
  // STDMETHOD(Code)(UInt32 size, UInt32 &processedSize);
  // STDMETHOD(Flush)();

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);


  CCoderMixer();
  ~CCoderMixer();
  void AddCoder(ICompressCoder *aCoder);
  void FinishAddingCoders();

  void ReInit();
  void SetCoderInfo(UInt32 coderIndex, 
      const UInt64 *inSize, const UInt64 *outSize);
  void SetProgressCoderIndex(UInt32 coderIndex)
    {  m_ProgressCoderIndex = coderIndex; }
  UInt64 GetWriteProcessedSize(UInt32 coderIndex);

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
  UInt32 m_ProgressCoderIndex;
};

#endif

