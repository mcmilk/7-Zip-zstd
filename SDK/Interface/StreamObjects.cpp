// StreamObjects.cpp

#include "StdAfx.h"

#include "StreamObjects.h"

void COutStreamImp::Init()
{
  m_Size = 0;
}

STDMETHODIMP COutStreamImp::Read(void *aData, ULONG aSize, ULONG *aProcessedSize)
  {  return E_NOTIMPL; }

STDMETHODIMP COutStreamImp::Write(void const *aData, ULONG aSize, ULONG *aProcessedSize)
{
  size_t aNewCapacity = m_Size + aSize;
  m_Buffer.EnsureCapacity(aNewCapacity);
  memmove(m_Buffer + m_Size, aData, aSize);
  if(aProcessedSize != NULL)
    *aProcessedSize = aSize;
  m_Size += aSize;
  return S_OK; 
}

void CInStreamImp::Init(BYTE *aDataPointer, UINT32 aSize)
{
  m_DataPointer = aDataPointer;
  m_Size = aSize;
  m_Pos = 0;
}

STDMETHODIMP CInStreamImp::Read(void *aData, ULONG aSize, ULONG *aProcessedSize)
{
  UINT32 aNumBytesToRead = MyMin(m_Pos + aSize, m_Size) - m_Pos;
  if(aProcessedSize != NULL)
    *aProcessedSize = aNumBytesToRead;
  memmove(aData, m_DataPointer + m_Pos, aNumBytesToRead);
  m_Pos += aNumBytesToRead;
  if(aNumBytesToRead == aSize)
    return S_OK;
  else
    return S_FALSE;
}

STDMETHODIMP CInStreamImp::Write(void const *aData, ULONG aSize, ULONG *aProcessedSize)
  {  return E_NOTIMPL; }



STDMETHODIMP CSequentialInStreamImp::Read(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aNumBytesToRead = MyMin(m_Pos + aSize, m_Size) - m_Pos;
  memmove(aData, m_DataPointer + m_Pos, aNumBytesToRead);
  m_Pos += aNumBytesToRead;
  if(aProcessedSize != NULL)
    *aProcessedSize = aNumBytesToRead;
  return S_OK;
}

STDMETHODIMP CSequentialInStreamImp::ReadPart(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return Read(aData, aSize, aProcessedSize);
}

////////////////////


void CWriteBuffer::Write(const void *aData, UINT32 aSize)
{
  size_t aNewCapacity = m_Size + aSize;
  m_Buffer.EnsureCapacity(aNewCapacity);
  memmove(m_Buffer + m_Size, aData, aSize);
  m_Size += aSize;
}

STDMETHODIMP CSequentialOutStreamImp::Write(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  m_WriteBuffer.Write(aData, aSize);
  if(aProcessedSize != NULL)
    *aProcessedSize = aSize;
  return S_OK; 
}

STDMETHODIMP CSequentialOutStreamImp::WritePart(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return Write(aData, aSize, aProcessedSize);
}

STDMETHODIMP CSequentialOutStreamImp2::Write(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aNewSize = aSize;
  if (m_Pos + aSize > m_Size)
    aNewSize = m_Size - m_Pos;
  memmove(m_Buffer + m_Pos, aData, aNewSize);
  if(aProcessedSize != NULL)
    *aProcessedSize = aNewSize;
  m_Pos += aNewSize;
  if (aNewSize != aSize)
    return E_FAIL;
  return S_OK; 
}

STDMETHODIMP CSequentialOutStreamImp2::WritePart(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return Write(aData, aSize, aProcessedSize);
}



STDMETHODIMP CSequentialInStreamSizeCount::Read(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->Read(aData, aSize, &aProcessedSizeReal);
  m_Size += aProcessedSizeReal;
  if (aProcessedSize != 0)
    *aProcessedSize = aProcessedSizeReal;
  return aResult; 
}

STDMETHODIMP CSequentialInStreamSizeCount::ReadPart(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->ReadPart(aData, aSize, &aProcessedSizeReal);
  m_Size += aProcessedSizeReal;
  if (aProcessedSize != 0)
    *aProcessedSize = aProcessedSizeReal;
  return aResult; 
}


STDMETHODIMP CSequentialOutStreamSizeCount::Write(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->Write(aData, aSize, &aProcessedSizeReal);
  m_Size += aProcessedSizeReal;
  if (aProcessedSize != 0)
    *aProcessedSize = aProcessedSizeReal;
  return aResult; 
}

STDMETHODIMP CSequentialOutStreamSizeCount::WritePart(const void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = m_Stream->WritePart(aData, aSize, &aProcessedSizeReal);
  m_Size += aProcessedSizeReal;
  if (aProcessedSize != 0)
    *aProcessedSize = aProcessedSizeReal;
  return aResult; 
}
