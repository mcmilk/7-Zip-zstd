// Windows/Memory.cpp

#include "StdAfx.h"

#include "Windows/Memory.h"

namespace NWindows {
namespace NMemory {

bool CGlobal::Alloc(UINT flags, SIZE_T size)
{
  HGLOBAL newBlock = ::GlobalAlloc(flags, size);
  if (newBlock == NULL)
    return false;
  m_MemoryHandle = newBlock;
  return true;
}

bool CGlobal::Free()
{
  if (m_MemoryHandle == NULL)
    return true;
  m_MemoryHandle = ::GlobalFree(m_MemoryHandle);
  return (m_MemoryHandle == NULL);
}

bool CGlobal::ReAlloc(SIZE_T size)
{
  HGLOBAL newBlock = ::GlobalReAlloc(m_MemoryHandle, size, GMEM_MOVEABLE);
  if (newBlock == NULL)
    return false;
  m_MemoryHandle = newBlock;
  return true;
}

}}
