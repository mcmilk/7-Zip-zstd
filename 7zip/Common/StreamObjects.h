// StreamObjects.h

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
  UInt32 _size;
public:
  COutStreamImp(): _size(0) {}
  void Init(){ _size = 0; }
  UInt32 GetSize() const { return _size; }
  const CByteDynamicBuffer& GetBuffer() const { return _buffer; }

  MY_UNKNOWN_IMP

  STDMETHODIMP Read(void *data, ULONG size, ULONG *processedSize);
  STDMETHODIMP Write(void const *data, ULONG size, ULONG *processedSize);
};

class CInStreamImp: 
  public ISequentialStream,
  public CMyUnknownImp
{
  Byte *_dataPointer;
  UInt32 _size;
  UInt32 _pos;

public:
  CInStreamImp(): _size(0xFFFFFFFF), _pos(0), _dataPointer(NULL) {}
  void Init(Byte *dataPointer, UInt32 size);

  MY_UNKNOWN_IMP

  STDMETHODIMP Read(void *data, ULONG size, ULONG *processedSize);
  STDMETHODIMP Write(void const *data, ULONG size, ULONG *processedSize);
};

class CSequentialInStreamImp: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  const Byte *_dataPointer;
  UInt32 _size;
  UInt32 _pos;

public:
  void Init(const Byte *dataPointer, UInt32 size)
  {
    _dataPointer = dataPointer;
    _size = size;
    _pos = 0;
  }

  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UInt32 size, UInt32 *processedSize);
};


class CWriteBuffer
{
  CByteDynamicBuffer _buffer;
  UInt32 _size;
public:
  CWriteBuffer(): _size(0) {}
  // void Init(UInt32 size = 0)  
  void Init()  
  { 
    /*
    if (size > 0)
      _buffer.EnsureCapacity(size);
    */
    _size = 0; 
  }
  void Write(const void *data, UInt32 size);
  UInt32 GetSize() const { return _size; }
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
  void Init(UInt32 size = 0)  
  { 
    _writeBuffer.Init(size);
  }
  */
  UInt32 GetSize() const { return _writeBuffer.GetSize(); }
  const CByteDynamicBuffer& GetBuffer() const { return _writeBuffer.GetBuffer(); }

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UInt32 size, UInt32 *processedSize);
};

class CSequentialOutStreamImp2: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  Byte *_buffer;
public:
  UInt32 _size;
  UInt32 _pos;

  void Init(Byte *buffer, UInt32 size)  
  { 
    _buffer = buffer;
    _pos = 0;
    _size = size; 
  }

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UInt32 size, UInt32 *processedSize);
};

class CSequentialInStreamSizeCount: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _size;
public:
  void Init(ISequentialInStream *stream)
  {
    _stream = stream;
    _size = 0;
  }
  UInt64 GetSize() const { return _size; }

  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UInt32 size, UInt32 *processedSize);
};

class CSequentialInStreamRollback: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialInStream> _stream;
  Byte *_buffer;
  UInt32 _bufferSize;
  UInt64 _size;

  UInt32 _currentSize;
  UInt32 _currentPos;
public:
  CSequentialInStreamRollback(UInt32 bufferSize):
    _bufferSize(bufferSize),
    _buffer(0)
  {
    _buffer = new Byte[bufferSize];
  }
  ~CSequentialInStreamRollback()
  {
    delete _buffer;
  }

  void Init(ISequentialInStream *stream)
  {
    _stream = stream;
    _size = 0;
    _currentSize = 0;
    _currentPos = 0;
  }
  UInt64 GetSize() const { return _size; }

  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UInt32 size, UInt32 *processedSize);
  HRESULT Rollback(UInt32 rollbackSize);
};

class CSequentialOutStreamSizeCount: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
public:
  void Init(ISequentialOutStream *stream)
  {
    _stream = stream;
    _size = 0;
  }
  UInt64 GetSize() const { return _size; }

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UInt32 size, UInt32 *processedSize);
};

#endif
