// AlignedBuffer.cpp

#include "StdAfx.h"

#include "AlignedBuffer.h"

void *CAlignedBuffer::Allocate(size_t size, size_t mask)
{
  Free();
  m_Buffer = new unsigned char[size + mask];
  unsigned char *p = m_Buffer;
  while(((size_t)p & mask) != 0)
    p++;
  return (void *)p;
}

void CAlignedBuffer::Free()
{
  if (m_Buffer != 0)
    delete []m_Buffer;
  m_Buffer = 0;
}
