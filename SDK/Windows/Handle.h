// Windows/Handle.h

#pragma once

#ifndef __WINDOWS_HANDLE_H
#define __WINDOWS_HANDLE_H

namespace NWindows {

class CHandle
{
protected:
  HANDLE m_Handle;
public:
  operator HANDLE() { return m_Handle; }
  CHandle(): m_Handle(NULL) {}
  ~CHandle() { Close(); }
  bool Close()
  {
    if (m_Handle == NULL)
      return true;
    if (!::CloseHandle(m_Handle))
      return false;
    m_Handle = NULL;
    return true;
  }
  void Attach(HANDLE aHandle) 
    { m_Handle = aHandle; }
  HANDLE Detach() 
  { 
    HANDLE aHandle = m_Handle;
    m_Handle = NULL; 
    return m_Handle;
  }
};

}

#endif
