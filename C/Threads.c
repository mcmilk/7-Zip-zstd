/* Threads.c */

#include "Threads.h"
#include <process.h>

HRes GetError()
{
  DWORD res = GetLastError();
  return (res) ? (HRes)(res) : SZE_FAIL;
}

HRes BoolToHRes(int v) { return v ? SZ_OK : GetError(); }
HRes BOOLToHRes(BOOL v) { return v ? SZ_OK : GetError(); }

HRes MyCloseHandle(HANDLE *h)
{
  if (*h != NULL)
    if (!CloseHandle(*h))
      return GetError();
  *h = NULL;
  return SZ_OK;
}

HRes Thread_Create(CThread *thread, THREAD_FUNC_RET_TYPE (THREAD_FUNC_CALL_TYPE *startAddress)(void *), LPVOID parameter)
{ 
  unsigned threadId; /* Windows Me/98/95: threadId parameter may not be NULL in _beginthreadex/CreateThread functions */
  thread->handle = 
    /* CreateThread(0, 0, startAddress, parameter, 0, &threadId); */
    (HANDLE)_beginthreadex(NULL, 0, startAddress, parameter, 0, &threadId);
    /* maybe we must use errno here, but probably GetLastError() is also OK. */
  return BoolToHRes(thread->handle != 0);
}

HRes WaitObject(HANDLE h)
{
  return (HRes)WaitForSingleObject(h, INFINITE); 
}

HRes Thread_Wait(CThread *thread)
{
  if (thread->handle == NULL)
    return 1;
  return WaitObject(thread->handle); 
}

HRes Thread_Close(CThread *thread)
{
  return MyCloseHandle(&thread->handle);
}

HRes Event_Create(CEvent *p, BOOL manualReset, int initialSignaled)
{
  p->handle = CreateEvent(NULL, manualReset, (initialSignaled ? TRUE : FALSE), NULL);
  return BoolToHRes(p->handle != 0);
}

HRes ManualResetEvent_Create(CManualResetEvent *p, int initialSignaled)
  { return Event_Create(p, TRUE, initialSignaled); }
HRes ManualResetEvent_CreateNotSignaled(CManualResetEvent *p) 
  { return ManualResetEvent_Create(p, 0); }

HRes AutoResetEvent_Create(CAutoResetEvent *p, int initialSignaled)
  { return Event_Create(p, FALSE, initialSignaled); }
HRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *p) 
  { return AutoResetEvent_Create(p, 0); }

HRes Event_Set(CEvent *p) { return BOOLToHRes(SetEvent(p->handle)); }
HRes Event_Reset(CEvent *p) { return BOOLToHRes(ResetEvent(p->handle)); }
HRes Event_Wait(CEvent *p) { return WaitObject(p->handle); }
HRes Event_Close(CEvent *p) { return MyCloseHandle(&p->handle); }


HRes Semaphore_Create(CSemaphore *p, UInt32 initiallyCount, UInt32 maxCount)
{
  p->handle = CreateSemaphore(NULL, (LONG)initiallyCount, (LONG)maxCount, NULL);
  return BoolToHRes(p->handle != 0);
}

HRes Semaphore_Release(CSemaphore *p, LONG releaseCount, LONG *previousCount) 
{ 
  return BOOLToHRes(ReleaseSemaphore(p->handle, releaseCount, previousCount)); 
}
HRes Semaphore_ReleaseN(CSemaphore *p, UInt32 releaseCount)
{
  return Semaphore_Release(p, (LONG)releaseCount, NULL);
}
HRes Semaphore_Release1(CSemaphore *p)
{
  return Semaphore_ReleaseN(p, 1);
}

HRes Semaphore_Wait(CSemaphore *p) { return WaitObject(p->handle); }
HRes Semaphore_Close(CSemaphore *p) { return MyCloseHandle(&p->handle); }

HRes CriticalSection_Init(CCriticalSection *p)
{
  /* InitializeCriticalSection can raise only STATUS_NO_MEMORY exception */
  __try 
  { 
    InitializeCriticalSection(p); 
    /* InitializeCriticalSectionAndSpinCount(p, 0); */
  }  
  __except (EXCEPTION_EXECUTE_HANDLER) { return SZE_OUTOFMEMORY; }
  return SZ_OK;
}

