// StreamObjects.h

#pragma once

#ifndef __STREAMOBJECTS_H
#define __STREAMOBJECTS_H

#include "../../Common/DynamicBuffer.h"
#include "../../Common/MyCom.h"
#include "../IStream.h"

class COutStreamImp: 
  public ISequentialStream,
  public CMyUnknownImp
{
  CByteDynamicBuffer _buffer;
  UINT32 _size;
public:
  COutStreamImp(): _size(0) {}
  void Init(){ _size = 0; }
  UINT32 GetSize() const { return _size; }
  const CByteDynamicBuffer& GetBuffer() const { return _buffer; }

  MY_UNKNOWN_IMP

  STDMETHODIMP Read(void *data, ULONG size, ULONG *processedSize);
  STDMETHODIMP Write(void const *data, ULONG size, ULONG *processedSize);
};

class CInStreamImp: 
  public ISequentialStream,
  public CMyUnknownImp
{
  BYTE *_dataPointer;
  UINT32 _size;
  UINT32 _pos;

public:
  CInStreamImp(): _size(0xFFFFFFFF), _pos(0), _dataPointer(NULL) {}
  void Init(BYTE *dataPointer, UINT32 size);

  MY_UNKNOWN_IMP

  STDMETHODIMP Read(void *data, ULONG size, ULONG *processedSize);
  STDMETHODIMP Write(void const *data, ULONG size, ULONG *processedSize);
};

class CSequentialInStreamImp: 
  public ISequentialInStream,
  public CMyUnknownImp
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

  MY_UNKNOWN_IMP

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
  public CMyUnknownImp
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

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

class CSequentialOutStreamImp2: 
  public ISequentialOutStream,
  public CMyUnknownImp
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

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

class CSequentialInStreamSizeCount: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialInStream> _stream;
  UINT64 _size;
public:
  void Init(ISequentialInStream *stream)
  {
    _stream = stream;
    _size = 0;
  }
  UINT64 GetSize() const { return _size; }

  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
};

class CSequentialOutStreamSizeCount: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialOutStream> _stream;
  UINT64 _size;
public:
  void Init(ISequentialOutStream *stream)
  {
    _stream = stream;
    _size = 0;
  }
  UINT64 GetSize() const { return _size; }

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

#endif
