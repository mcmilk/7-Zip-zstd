// AlignedBuffer.cpp

#include "StdAfx.h"

#include "AlignedBuffer.h"
#include "Types.h"

void *CAlignedBuffer::Allocate(size_t numItems, size_t itemSize, size_t alignValue)
{
  Free();
  m_Buffer = new unsigned char[numItems * itemSize + alignValue - 1];
  UINT_PTR p = UINT_PTR(m_Buffer) + (alignValue - 1);
  p -= (p % alignValue);
  return (void *)p;
}

void CAlignedBuffer::Free()
{
  if (m_Buffer != 0)
    delete []m_Buffer;
  m_Buffer = 0;
}
