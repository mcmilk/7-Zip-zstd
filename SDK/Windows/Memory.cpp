// Windows/Memory.cpp

#include "StdAfx.h"

#include "Windows/Memory.h"

namespace NWindows {
namespace NMemory {

CGlobal::~CGlobal()
{
  Free();
}

// aFlags = GMEM_MOVEABLE
bool CGlobal::Alloc(UINT aFlags, DWORD aSize)
{
  HGLOBAL aNewBlock = ::GlobalAlloc(aFlags, aSize);
  if (aNewBlock == NULL)
    return false;
  m_MemoryHandle = aNewBlock;
  return true;
}

bool CGlobal::Free()
{
  if (m_MemoryHandle == NULL)
    return true;
  m_MemoryHandle = ::GlobalFree(m_MemoryHandle);
  return (m_MemoryHandle == NULL);
}

HGLOBAL CGlobal::Detach()
{
  HGLOBAL aHandle = m_MemoryHandle;
  m_MemoryHandle = NULL;
  return aHandle;
}

LPVOID CGlobal::Lock() const
{
  return ::GlobalLock(m_MemoryHandle);
}

void CGlobal::Unlock() const
{
  ::GlobalUnlock(m_MemoryHandle);
}

bool CGlobal::ReAlloc(DWORD aSize)
{
  HGLOBAL aNewBlock = ::GlobalReAlloc(m_MemoryHandle, GMEM_MOVEABLE, aSize);
  if (aNewBlock == NULL)
    return false;
  m_MemoryHandle = aNewBlock;
  return true;
}

}}
