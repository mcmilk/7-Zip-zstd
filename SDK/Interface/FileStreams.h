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
  NWindows::NFile::NIO::CInFile m_File;
  CInFileStream() {}
  bool Open(LPCTSTR aFileName);
  

BEGIN_COM_MAP(CInFileStream)
  COM_INTERFACE_ENTRY(IInStream)
  COM_INTERFACE_ENTRY(IStreamGetSize)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CInFileStream)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(Seek)(INT64 anOffset, UINT32 aSeekOrigin, UINT64 *aNewPosition);

  STDMETHOD(GetSize)(UINT64 *aSize);
};

class COutFileStream: 
  public IOutStream,
  public CComObjectRoot
{
public:
  NWindows::NFile::NIO::COutFile m_File;
  COutFileStream() {}
  bool Open(LPCTSTR aFileName);
  

BEGIN_COM_MAP(COutFileStream)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
  COM_INTERFACE_ENTRY(IOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COutFileStream)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(Seek)(INT64 anOffset, UINT32 aSeekOrigin, UINT64 *aNewPosition);
  STDMETHOD(SetSize)(INT64 aNewSize);
};

#endif