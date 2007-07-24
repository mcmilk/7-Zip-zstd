// Windows/Thread.h

#ifndef __WINDOWS_THREAD_H
#define __WINDOWS_THREAD_H

#include "Defs.h"

extern "C" 
{ 
#include "../../C/Threads.h"
}

namespace NWindows {

class CThread
{
  ::CThread thread;
public:
  CThread() { Thread_Construct(&thread); }
  ~CThread() { Close(); }
  bool IsCreated() { return Thread_WasCreated(&thread) != 0; }
  HRes Close()  { return Thread_Close(&thread); }
  HRes Create(THREAD_FUNC_RET_TYPE (THREAD_FUNC_CALL_TYPE *startAddress)(void *), LPVOID parameter)
    { return Thread_Create(&thread, startAddress, parameter); }
  HRes Wait() { return Thread_Wait(&thread); }
  
  #ifdef _WIN32
  DWORD Resume() { return ::ResumeThread(thread.handle); }
  DWORD Suspend() { return ::SuspendThread(thread.handle); }
  bool Terminate(DWORD exitCode) { return BOOLToBool(::TerminateThread(thread.handle, exitCode)); }
  int GetPriority() { return ::GetThreadPriority(thread.handle); }
  bool SetPriority(int priority) { return BOOLToBool(::SetThreadPriority(thread.handle, priority)); }
  #endif
};

}

#endif
