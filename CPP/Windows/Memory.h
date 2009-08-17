// Windows/Memory.h

#ifndef __WINDOWS_MEMORY_H
#define __WINDOWS_MEMORY_H

namespace NWindows {
namespace NMemory {

class CGlobal
{
  HGLOBAL m_MemoryHandle;
public:
  CGlobal(): m_MemoryHandle(NULL){};
  ~CGlobal() { Free(); }
  operator HGLOBAL() const { return m_MemoryHandle; };
  void Attach(HGLOBAL hGlobal)
  {
    Free();
    m_MemoryHandle = hGlobal;
  }
  HGLOBAL Detach()
  {
    HGLOBAL h = m_MemoryHandle;
    m_MemoryHandle = NULL;
    return h;
  }
  bool Alloc(UINT flags, SIZE_T size);
  bool Free();
  LPVOID Lock() const { return GlobalLock(m_MemoryHandle); }
  void Unlock() const { GlobalUnlock(m_MemoryHandle); }
  bool ReAlloc(SIZE_T size);
};

class CGlobalLock
{
  HGLOBAL m_Global;
  LPVOID m_Pointer;
public:
  LPVOID GetPointer() const { return m_Pointer; }
  CGlobalLock(HGLOBAL hGlobal): m_Global(hGlobal)
  {
    m_Pointer = GlobalLock(hGlobal);
  };
  ~CGlobalLock()
  {
    if (m_Pointer != NULL)
      GlobalUnlock(m_Global);
  }
};

}}

#endif
