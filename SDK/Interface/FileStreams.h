// FileStreams.h

#pragma once

#ifndef __FILESTREAMS_H
#define __FILESTREAMS_H

#include "Windows/FileIO.h"

#include "Interface/IInOutStreams.h"

class CInFileStream: 
  public IInStream,
  public IStreamGetSize,
  public CComObjectRoot
{
public:
  NWindows::NFile::NIO::CInFile File;
  CInFileStream() {}
  bool Open(LPCTSTR fileName);

BEGIN_COM_MAP(CInFileStream)
  COM_INTERFACE_ENTRY(IInStream)
  COM_INTERFACE_ENTRY(IStreamGetSize)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CInFileStream)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition);

  STDMETHOD(GetSize)(UINT64 *size);
};

class COutFileStream: 
  public IOutStream,
  public CComObjectRoot
{
public:
  NWindows::NFile::NIO::COutFile File;
  COutFileStream() {}
  bool Open(LPCTSTR fileName);
  

BEGIN_COM_MAP(COutFileStream)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
  COM_INTERFACE_ENTRY(IOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COutFileStream)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(Seek)(INT64 offset, UINT32 seekOrigin, UINT64 *newPosition);
  STDMETHOD(SetSize)(INT64 newSize);
};

#endif