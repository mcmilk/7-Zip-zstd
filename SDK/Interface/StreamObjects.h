// StreamObjects.h

#pragma once

#ifndef __STREAMOBJECTS_H
#define __STREAMOBJECTS_H

#include "Common/DynamicBuffer.h"
#include "Interface/IInOutStreams.h"

class COutStreamImp: 
  public ISequentialStream,
  public CComObjectRoot
{
  CByteDynamicBuffer _buffer;
  UINT32 _size;
public:
  COutStreamImp(): _size(0) {}
  void Init();
  UINT32 GetSize() const { return _size; }
  const CByteDynamicBuffer& GetBuffer() const { return _buffer; }


BEGIN_COM_MAP(COutStreamImp)
  COM_INTERFACE_ENTRY(ISequentialStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COutStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHODIMP Read(void *data, ULONG size, ULONG *processedSize);
  STDMETHODIMP Write(void const *data, ULONG size, ULONG *processedSize);
};

class CInStreamImp: 
  public ISequentialStream,
  public CComObjectRoot
{
  BYTE *_dataPointer;
  UINT32 _size;
  UINT32 _pos;

public:
  CInStreamImp(): _size(0xFFFFFFFF), _pos(0), _dataPointer(NULL) {}
  void Init(BYTE *dataPointer, UINT32 size);

BEGIN_COM_MAP(CInStreamImp)
  COM_INTERFACE_ENTRY(ISequentialStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CInStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHODIMP Read(void *data, ULONG size, ULONG *processedSize);
  STDMETHODIMP Write(void const *data, ULONG size, ULONG *processedSize);
};

class CSequentialInStreamImp: 
  public ISequentialInStream,
  public CComObjectRoot
{
  const BYTE *_dataPointer;
  UINT32 _size;
  UINT32 _pos;

public:
  void Init(const BYTE *dataPointer, UINT32 size)
  {
    _dataPointer = dataPointer;
    _size = size;
    _pos = 0;
  }

BEGIN_COM_MAP(CSequentialInStreamImp)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialInStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
};


class CWriteBuffer
{
  CByteDynamicBuffer _buffer;
  UINT32 _size;
public:
  CWriteBuffer(): _size(0) {}
  // void Init(UINT32 size = 0)  
  void Init()  
  { 
    /*
    if (size > 0)
      _buffer.EnsureCapacity(size);
    */
    _size = 0; 
  }
  void Write(const void *data, UINT32 size);
  UINT32 GetSize() const { return _size; }
  const CByteDynamicBuffer& GetBuffer() const { return _buffer; }
};

class CSequentialOutStreamImp: 
  public ISequentialOutStream,
  public CComObjectRoot
{
  CWriteBuffer _writeBuffer;
public:
  void Init()
  {
    _writeBuffer.Init();
  }

  /*
  void Init(UINT32 size = 0)  
  { 
    _writeBuffer.Init(size);
  }
  */
  UINT32 GetSize() const { return _writeBuffer.GetSize(); }
  const CByteDynamicBuffer& GetBuffer() const { return _writeBuffer.GetBuffer(); }

BEGIN_COM_MAP(CSequentialOutStreamImp)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialOutStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

class CSequentialOutStreamImp2: 
  public ISequentialOutStream,
  public CComObjectRoot
{
  BYTE *_buffer;
public:
  UINT32 _size;
  UINT32 _pos;

  void Init(BYTE *buffer, UINT32 size)  
  { 
    _buffer = buffer;
    _pos = 0;
    _size = size; 
  }
BEGIN_COM_MAP(CSequentialOutStreamImp2)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialOutStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

class CSequentialInStreamSizeCount: 
  public ISequentialInStream,
  public CComObjectRoot
{
  CComPtr<ISequentialInStream> _stream;
  UINT64 _size;
public:
  void Init(ISequentialInStream *stream)
  {
    _stream = stream;
    _size = 0;
  }
  UINT64 GetSize() const { return _size; }
BEGIN_COM_MAP(CSequentialInStreamSizeCount)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialInStreamSizeCount)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
};

class CSequentialOutStreamSizeCount: 
  public ISequentialOutStream,
  public CComObjectRoot
{
  CComPtr<ISequentialOutStream> _stream;
  UINT64 _size;
public:
  void Init(ISequentialOutStream *stream)
  {
    _stream = stream;
    _size = 0;
  }
  UINT64 GetSize() const { return _size; }
BEGIN_COM_MAP(CSequentialOutStreamSizeCount)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialOutStreamSizeCount)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

#endif