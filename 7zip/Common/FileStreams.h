// FileStreams.h

#pragma once

#ifndef __FILESTREAMS_H
#define __FILESTREAMS_H

#include "Windows/FileIO.h"

#include "../IStream.h"
#include "Common/MyCom.h"

class CInFileStream: 
  public IInStream,
  public IStreamGetSize,
  public CMyUnknownImp
{
public:
  NWindows::NFile::NIO::CInFile File;
  CInFileStream() {}
  bool Open(LPCTSTR fileName);

  MY_UNKNOWN_IMP1(IStreamGetSize)

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition);

  STDMETHOD(GetSize)(UINT64 *size);
};

class COutFileStream: 
  public IOutStream,
  public CMyUnknownImp
{
public:
  NWindows::NFile::NIO::COutFile File;
  COutFileStream() {}
  bool Open(LPCTSTR fileName);
  
  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition);
  STDMETHOD(SetSize)(INT64 newSize);
};

#endif