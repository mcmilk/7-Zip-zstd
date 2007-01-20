/* Thresds.h */

#ifndef __7Z_THRESDS_H
#define __7Z_THRESDS_H

#include <windows.h>

#include "Types.h"

typedef struct _CThread
{
  HANDLE handle;
} CThread;

#define Thread_Construct(thread) (thread)->handle = NULL
#define Thread_WasCreated(thread) ((thread)->handle != NULL)
 
HRes Thread_Create(CThread *thread, unsigned (StdCall *startAddress)(void *), LPVOID parameter);
HRes Thread_Wait(CThread *thread);
HRes Thread_Close(CThread *thread);

typedef struct _CEvent
{
  HANDLE handle;
} CEvent;

typedef CEvent CAutoResetEvent;

#define Event_Construct(event) (event)->handle = NULL

HRes AutoResetEvent_Create(CAutoResetEvent *event, int initialSignaled);
HRes AutoResetEvent_CreateNotSignaled(CAutoResetEvent *event);
HRes Event_Set(CEvent *event);
HRes Event_Reset(CEvent *event);
HRes Event_Wait(CEvent *event);
HRes Event_Close(CEvent *event);


typedef struct _CSemaphore
{
  HANDLE handle;
} CSemaphore;

#define Semaphore_Construct(p) (p)->handle = NULL

HRes Semaphore_Create(CSemaphore *p, UInt32 initiallyCount, UInt32 maxCount);
HRes Semaphore_Release1(CSemaphore *p);
HRes Semaphore_Wait(CSemaphore *p);
HRes Semaphore_Close(CSemaphore *p);


typedef CRITICAL_SECTION CCriticalSection;

HRes CriticalSection_Init(CCriticalSection *p);
#define CriticalSection_Delete(p) DeleteCriticalSection(p)
#define CriticalSection_Enter(p) EnterCriticalSection(p)
#define CriticalSection_Leave(p) LeaveCriticalSection(p)

#endif

