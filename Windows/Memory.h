// Windows/Memory.h

#ifndef __WINDOWS_MEMORY_H
#define __WINDOWS_MEMORY_H

namespace NWindows {
namespace NMemory {

class CGlobal
{
  HGLOBAL m_MemoryHandle;
public:
  CGlobal():m_MemoryHandle(NULL){};
  ~CGlobal();
  operator HGLOBAL() const { return m_MemoryHandle; };
  HGLOBAL Detach();
  bool Alloc(UINT aFlags, DWORD aSize);
  bool Free();
  LPVOID Lock() const;
  void Unlock() const;
  bool ReAlloc(DWORD aSize);
};


class CGlobalLock
{
  const HGLOBAL m_Global;
  LPVOID m_Pointer;
public:
  LPVOID GetPointer() const { return m_Pointer; }
  CGlobalLock(HGLOBAL aGlobal): m_Global(aGlobal)
  {
    m_Pointer = ::GlobalLock(m_Global); 
  };
  ~CGlobalLock()
  {
    if(m_Pointer != NULL)
      ::GlobalUnlock(m_Global);
  }
};

}}

#endif
