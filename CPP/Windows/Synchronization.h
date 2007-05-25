// Windows/Synchronization.h

#ifndef __WINDOWS_SYNCHRONIZATION_H
#define __WINDOWS_SYNCHRONIZATION_H

#include "Defs.h"

extern "C" 
{ 
#include "../../C/Threads.h"
}

#ifdef _WIN32
#include "Handle.h"
#endif

namespace NWindows {
namespace NSynchronization {

class CBaseEvent
{
protected:
  ::CEvent _object;
public:
  bool IsCreated() { return Event_IsCreated(&_object) != 0; }
  operator HANDLE() { return _object.handle; }
  CBaseEvent() { Event_Construct(&_object); }
  ~CBaseEvent() { Close(); }
  HRes Close() { return Event_Close(&_object); }
  #ifdef _WIN32
  HRes Create(bool manualReset, bool initiallyOwn, LPCTSTR name = NULL,
      LPSECURITY_ATTRIBUTES securityAttributes = NULL)
  {
    _object.handle = ::CreateEvent(securityAttributes, BoolToBOOL(manualReset),
        BoolToBOOL(initiallyOwn), name);
    if (_object.handle != 0)
      return 0;
    return ::GetLastError();
  }
  HRes Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
  {
    _object.handle = ::OpenEvent(desiredAccess, BoolToBOOL(inheritHandle), name);
    if (_object.handle != 0)
      return 0;
    return ::GetLastError();
  }
  #endif

  HRes Set() { return Event_Set(&_object); }
  // bool Pulse() { return BOOLToBool(::PulseEvent(_handle)); }
  HRes Reset() { return Event_Reset(&_object); }
  HRes Lock() { return Event_Wait(&_object); }
};

class CManualResetEvent: public CBaseEvent
{
public:
  HRes Create(bool initiallyOwn = false)
  {
    return ManualResetEvent_Create(&_object, initiallyOwn ? 1: 0);
  }
  HRes CreateIfNotCreated()
  {
    if (IsCreated())
      return 0;
    return ManualResetEvent_CreateNotSignaled(&_object);
  }
  #ifdef _WIN32
  HRes CreateWithName(bool initiallyOwn, LPCTSTR name)
  {
    return CBaseEvent::Create(true, initiallyOwn, name);
  }
  #endif
};

class CAutoResetEvent: public CBaseEvent
{
public:
  HRes Create()
  {
    return AutoResetEvent_CreateNotSignaled(&_object);
  }
  HRes CreateIfNotCreated()
  {
    if (IsCreated())
      return 0;
    return AutoResetEvent_CreateNotSignaled(&_object);
  }
};

#ifdef _WIN32
class CObject: public CHandle
{
public:
  HRes Lock(DWORD timeoutInterval = INFINITE)
    { return (::WaitForSingleObject(_handle, timeoutInterval) == WAIT_OBJECT_0 ? 0 : ::GetLastError()); }
};
class CMutex: public CObject
{
public:
  HRes Create(bool initiallyOwn, LPCTSTR name = NULL,
      LPSECURITY_ATTRIBUTES securityAttributes = NULL)
  {
    _handle = ::CreateMutex(securityAttributes, BoolToBOOL(initiallyOwn), name);
    if (_handle != 0)
      return 0;
    return ::GetLastError();
  }
  HRes Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
  {
    _handle = ::OpenMutex(desiredAccess, BoolToBOOL(inheritHandle), name);
    if (_handle != 0)
      return 0;
    return ::GetLastError();
  }
  HRes Release() 
  { 
    return ::ReleaseMutex(_handle) ? 0 : ::GetLastError();
  }
};
class CMutexLock
{
  CMutex *_object;
public:
  CMutexLock(CMutex &object): _object(&object) { _object->Lock(); } 
  ~CMutexLock() { _object->Release(); }
};
#endif

class CSemaphore
{
  ::CSemaphore _object;
public:
  CSemaphore() { Semaphore_Construct(&_object); }
  ~CSemaphore() { Close(); }
  HRes Close() {  return Semaphore_Close(&_object); }
  operator HANDLE() { return _object.handle; }
  HRes Create(UInt32 initiallyCount, UInt32 maxCount)
  {
    return Semaphore_Create(&_object, initiallyCount, maxCount);
  }
  HRes Release() { return Semaphore_Release1(&_object); }
  HRes Release(UInt32 releaseCount) { return Semaphore_ReleaseN(&_object, releaseCount); }
  HRes Lock() { return Semaphore_Wait(&_object); }
};

class CCriticalSection
{
  ::CCriticalSection _object;
public:
  CCriticalSection() { CriticalSection_Init(&_object); }
  ~CCriticalSection() { CriticalSection_Delete(&_object); }
  void Enter() { CriticalSection_Enter(&_object); }
  void Leave() { CriticalSection_Leave(&_object); }
};

class CCriticalSectionLock
{
  CCriticalSection *_object;
  void Unlock()  { _object->Leave(); }
public:
  CCriticalSectionLock(CCriticalSection &object): _object(&object) {_object->Enter(); } 
  ~CCriticalSectionLock() { Unlock(); }
};

}}

#endif
