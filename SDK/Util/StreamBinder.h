// StreamBinder.h

#pragma once

#ifndef __STREAMBINDER_H
#define __STREAMBINDER_H

#include "Interface/IInOutStreams.h"
#include "Windows/Synchronization.h"

class CStreamBinder
{
  NWindows::NSynchronization::CManualResetEvent *m_AllBytesAreWritenEvent;
  NWindows::NSynchronization::CManualResetEvent *m_ThereAreBytesToReadEvent;
  NWindows::NSynchronization::CManualResetEvent *m_ReadStreamIsClosedEvent;
  UINT32 m_BufferSize;
  const void *m_Buffer;
public:
  // bool ReadingWasClosed;
  UINT64 m_ProcessedSize;
  CStreamBinder():
    m_AllBytesAreWritenEvent(NULL), 
    m_ThereAreBytesToReadEvent(NULL),
    m_ReadStreamIsClosedEvent(NULL)
    {}
  ~CStreamBinder();
  void CreateEvents();

  void CreateStreams(ISequentialInStream **anInStream, 
      ISequentialOutStream **anOutStream);
  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  void CloseRead();

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  void CloseWrite();
  void ReInit();
};

#endif

