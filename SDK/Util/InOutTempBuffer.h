// Util/InOutTempBuffer.h

#pragma once

#ifndef __INOUTTEMPBUFFER_H
#define __INOUTTEMPBUFFER_H

#include "Windows/FileIO.h"
#include "Windows/FileDir.h"

#include "Interface/IInOutStreams.h"

class CInOutTempBuffer
{
  NWindows::NFile::NDirectory::CTempFile m_TempFile;
  NWindows::NFile::NIO::COutFile m_OutFile;
  NWindows::NFile::NIO::CInFile m_InFile;
  BYTE *m_Buffer;
  UINT32 m_BufferPosition;
  UINT32 m_CurrentPositionInBuffer;
  CSysString m_TmpFileName;
  bool m_TmpFileCreated;

  UINT64 m_FileSize;

  bool WriteToFile(const void *aPointer, UINT32 aSize);
public:
  CInOutTempBuffer();
  ~CInOutTempBuffer();
  void Create();

  void InitWriting();
  bool Write(const void *aPointer, UINT32 aSize);
  UINT64 GetDataSize() const { return m_FileSize; }
  bool FlushWrite();
  bool InitReading();
  // bool Read(void *aPointer, UINT32 aMaxSize, UINT32 &aProcessedSize);
  HRESULT WriteToStream(ISequentialOutStream *aStream);
};

class CSequentialOutTempBufferImp: 
  public ISequentialOutStream,
  public CComObjectRoot
{
  CInOutTempBuffer *m_Buffer;
public:
  // CSequentialOutStreamImp(): m_Size(0) {}
  // UINT32 m_Size;
  void Init(CInOutTempBuffer *aBuffer)  { m_Buffer = aBuffer; }
  // UINT32 GetSize() const { return m_Size; }

BEGIN_COM_MAP(CSequentialOutTempBufferImp)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialOutTempBufferImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};


#endif