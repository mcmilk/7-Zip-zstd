// Windows/Th read.h

#pragma once

#ifndef __WINDOWS_THREAD_H
#define __WINDOWS_THREAD_H

#include "Windows/Handle.h"
#include "Windows/Defs.h"

namespace NWindows {

class CThread: public CHandle
{
public:
  bool Create(LPSECURITY_ATTRIBUTES aThreadAttributes, 
      SIZE_T aStackSize, LPTHREAD_START_ROUTINE aStartAddress,
      LPVOID aParameter, DWORD aCreationFlags, LPDWORD aThreadId)
  {
    m_Handle = ::CreateThread(aThreadAttributes, aStackSize, aStartAddress,
        aParameter, aCreationFlags, aThreadId);
    return (m_Handle != NULL);
  }
  bool Create(LPTHREAD_START_ROUTINE aStartAddress, LPVOID aParameter)
  {
    DWORD aThreadId;
    return Create(NULL, 0, aStartAddress, aParameter, 0, &aThreadId);
  }
  
  DWORD Resume()
    { return ::ResumeThread(m_Handle); }
  DWORD Suspend()
    { return ::SuspendThread(m_Handle); }
  bool Terminate(DWORD anExitCode)
    { return BOOLToBool(::TerminateThread(m_Handle, anExitCode)); }
  
  int GetPriority()
    { return ::GetThreadPriority(m_Handle); }
  bool SetPriority(int aPriority)
    { return BOOLToBool(::SetThreadPriority(m_Handle, aPriority)); }
};

}

#endif
