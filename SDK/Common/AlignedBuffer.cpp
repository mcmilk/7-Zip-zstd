// AlignedBuffer.cpp

#include "StdAfx.h"

#include "AlignedBuffer.h"

CAlignedBuffer::~CAlignedBuffer()
{
  Free();
}

void *CAlignedBuffer::Allocate(size_t aNumItems, size_t anItemSize, size_t anAlignValue)
{
  Free();
  m_Buffer = new unsigned char[aNumItems * anItemSize + anAlignValue - 1];
  UINT_PTR aPointer = UINT_PTR(m_Buffer) + (anAlignValue - 1);
  aPointer -= (aPointer % anAlignValue);
  return (void *)aPointer;
}

void CAlignedBuffer::Free()
{
  if (m_Buffer != 0)
    delete []m_Buffer;
  m_Buffer = 0;
}
