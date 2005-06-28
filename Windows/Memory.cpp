// Windows/Memory.cpp

#include "StdAfx.h"

#include "Windows/Memory.h"

namespace NWindows {
namespace NMemory {

CGlobal::~CGlobal()
{
  Free();
}

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

void CGlobal::Attach(HGLOBAL hGlobal)
{
  Free();
  m_MemoryHandle = hGlobal;
}

HGLOBAL CGlobal::Detach()
{
  HGLOBAL h = m_MemoryHandle;
  m_MemoryHandle = NULL;
  return h;
}

LPVOID CGlobal::Lock() const
{
  return ::GlobalLock(m_MemoryHandle);
}

void CGlobal::Unlock() const
{
  ::GlobalUnlock(m_MemoryHandle);
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
