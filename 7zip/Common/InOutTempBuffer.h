// Util/InOutTempBuffer.h

#pragma once

#ifndef __INOUTTEMPBUFFER_H
#define __INOUTTEMPBUFFER_H

#include "../../Windows/FileIO.h"
#include "../../Windows/FileDir.h"
#include "../../Common/MyCom.h"

#include "../IStream.h"

class CInOutTempBuffer
{
  NWindows::NFile::NDirectory::CTempFile _tempFile;
  NWindows::NFile::NIO::COutFile _outFile;
  NWindows::NFile::NIO::CInFile _inFile;
  BYTE *_buffer;
  UINT32 _bufferPosition;
  UINT32 _currentPositionInBuffer;
  CSysString _tmpFileName;
  bool _tmpFileCreated;

  UINT64 _fileSize;

  bool WriteToFile(const void *data, UINT32 size);
public:
  CInOutTempBuffer();
  ~CInOutTempBuffer();
  void Create();

  void InitWriting();
  bool Write(const void *data, UINT32 size);
  UINT64 GetDataSize() const { return _fileSize; }
  bool FlushWrite();
  bool InitReading();
  // bool Read(void *data, UINT32 maxSize, UINT32 &processedSize);
  HRESULT WriteToStream(ISequentialOutStream *stream);
};

class CSequentialOutTempBufferImp: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  CInOutTempBuffer *_buffer;
public:
  // CSequentialOutStreamImp(): _size(0) {}
  // UINT32 _size;
  void Init(CInOutTempBuffer *buffer)  { _buffer = buffer; }
  // UINT32 GetSize() const { return _size; }

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};


#endif