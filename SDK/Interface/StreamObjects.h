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
  CByteDynamicBuffer m_Buffer;
  UINT32 m_Size;
public:
  COutStreamImp(): m_Size(0) {}
  void Init();
  UINT32 GetSize() const { return m_Size; }
  const CByteDynamicBuffer& GetBuffer() const { return m_Buffer; }


BEGIN_COM_MAP(COutStreamImp)
  COM_INTERFACE_ENTRY(ISequentialStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(COutStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHODIMP Read(void *aData, ULONG aSize, ULONG *aProccessedSize);
  STDMETHODIMP Write(void const *aData, ULONG aSize, ULONG *aProccessedSize);
};

class CInStreamImp: 
  public ISequentialStream,
  public CComObjectRoot
{
  BYTE *m_DataPointer;
  UINT32 m_Size;
  UINT32 m_Pos;

public:
  CInStreamImp(): m_Size(0xFFFFFFFF), m_Pos(0), m_DataPointer(NULL) {}
  void Init(BYTE *aDataPointer, UINT32 aSize);

BEGIN_COM_MAP(CInStreamImp)
  COM_INTERFACE_ENTRY(ISequentialStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CInStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHODIMP Read(void *aData, ULONG aSize, ULONG *aProccessedSize);
  STDMETHODIMP Write(void const *aData, ULONG aSize, ULONG *aProccessedSize);
};

class CSequentialInStreamImp: 
  public ISequentialInStream,
  public CComObjectRoot
{
  const BYTE *m_DataPointer;
  UINT32 m_Size;
  UINT32 m_Pos;

public:
  void Init(const BYTE *aDataPointer, UINT32 aSize)
  {
    m_DataPointer = aDataPointer;
    m_Size = aSize;
    m_Pos = 0;
  }

BEGIN_COM_MAP(CSequentialInStreamImp)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialInStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};


class CWriteBuffer
{
  CByteDynamicBuffer m_Buffer;
  UINT32 m_Size;
public:
  CWriteBuffer(): m_Size(0) {}
  // void Init(UINT32 aSize = 0)  
  void Init()  
  { 
    /*
    if (aSize > 0)
      m_Buffer.EnsureCapacity(aSize);
    */
    m_Size = 0; 
  }
  void Write(const void *aData, UINT32 aSize);
  UINT32 GetSize() const { return m_Size; }
  const CByteDynamicBuffer& GetBuffer() const { return m_Buffer; }
};

class CSequentialOutStreamImp: 
  public ISequentialOutStream,
  public CComObjectRoot
{
  CWriteBuffer m_WriteBuffer;
public:
  void Init()
  {
    m_WriteBuffer.Init();
  }

  /*
  void Init(UINT32 aSize = 0)  
  { 
    m_WriteBuffer.Init(aSize);
  }
  */
  UINT32 GetSize() const { return m_WriteBuffer.GetSize(); }
  const CByteDynamicBuffer& GetBuffer() const { return m_WriteBuffer.GetBuffer(); }

BEGIN_COM_MAP(CSequentialOutStreamImp)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialOutStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};

class CSequentialOutStreamImp2: 
  public ISequentialOutStream,
  public CComObjectRoot
{
  BYTE *m_Buffer;
public:
  UINT32 m_Size;
  UINT32 m_Pos;

  void Init(BYTE *aBuffer, UINT32 aSize)  
  { 
    m_Buffer = aBuffer;
    m_Pos = 0;
    m_Size = aSize; 
  }
BEGIN_COM_MAP(CSequentialOutStreamImp2)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialOutStreamImp)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};

class CSequentialInStreamSizeCount: 
  public ISequentialInStream,
  public CComObjectRoot
{
  CComPtr<ISequentialInStream> m_Stream;
  UINT64 m_Size;
public:
  void Init(ISequentialInStream *aStream)
  {
    m_Stream = aStream;
    m_Size = 0;
  }
  UINT64 GetSize() const { return m_Size; }
BEGIN_COM_MAP(CSequentialInStreamSizeCount)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialInStreamSizeCount)

DECLARE_NO_REGISTRY()

  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};

class CSequentialOutStreamSizeCount: 
  public ISequentialOutStream,
  public CComObjectRoot
{
  CComPtr<ISequentialOutStream> m_Stream;
  UINT64 m_Size;
public:
  void Init(ISequentialOutStream *aStream)
  {
    m_Stream = aStream;
    m_Size = 0;
  }
  UINT64 GetSize() const { return m_Size; }
BEGIN_COM_MAP(CSequentialOutStreamSizeCount)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSequentialOutStreamSizeCount)

DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
};

#endif