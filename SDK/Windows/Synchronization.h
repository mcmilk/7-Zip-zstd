// Windows/Synchronization.h

#pragma once

#ifndef __WINDOWS_SYNCHRONIZATION_H
#define __WINDOWS_SYNCHRONIZATION_H

#include "Windows/Defs.h"
#include "Windows/Handle.h"

namespace NWindows {
namespace NSynchronization {

class CSingleLock;

class CObject: public CHandle
{
public:
  bool Lock(DWORD timeoutInterval = INFINITE)
    { return (::WaitForSingleObject(_handle, timeoutInterval) == WAIT_OBJECT_0); }
};

class CSyncObject
{
public:
  HANDLE  _object;

  CSyncObject(LPCTSTR name): _object(NULL) {};
  operator HANDLE() const
    { return _object;}

  virtual bool Lock(DWORD timeoutInterval = INFINITE);
  virtual bool Unlock() = 0;
  virtual bool Unlock(LONG /* count */, LPLONG /* prevCount = NULL */)
    { return true; }

public:
  virtual ~CSyncObject();
  friend class CSingleLock;
};

class CEvent: public CObject
{
public:
  CEvent() {};
  CEvent(bool manualReset, bool initiallyOwn, 
      LPCTSTR name = NULL, LPSECURITY_ATTRIBUTES securityAttributes = NULL);

  bool Create(bool manualReset, bool initiallyOwn, LPCTSTR name = NULL,
      LPSECURITY_ATTRIBUTES securityAttributes = NULL)
  {
    _handle = ::CreateEvent(securityAttributes, BoolToBOOL(manualReset),
      BoolToBOOL(initiallyOwn), name);
    return (_handle != 0);
  }

  bool Open(DWORD desiredAccess, bool inheritHandle, LPCTSTR name)
  {
    _handle = ::OpenEvent(desiredAccess, BoolToBOOL(inheritHandle), name);
    return (_handle != 0);
  }

  bool Set() { return BOOLToBool(::SetEvent(_handle)); }
  bool Pulse() { return BOOLToBool(::PulseEvent(_handle)); }
  bool Reset() { return BOOLToBool(::ResetEvent(_handle)); }
};

class CManualResetEvent: public CEvent
{
public:
  CManualResetEvent(bool initiallyOwn = false, LPCTSTR name = NULL, 
      LPSECURITY_ATTRIBUTES securityAttributes = NULL):
    CEvent(true, initiallyOwn, name, securityAttributes) {};
};

class CAutoResetEvent: public CEvent
{
public:
  CAutoResetEvent(bool initiallyOwn = false, LPCTSTR name = NULL, 
      LPSECURITY_ATTRIBUTES securityAttributes = NULL):
    CEvent(false, initiallyOwn, name, securityAttributes) {};
};


class CCriticalSection: public CSyncObject
{
  CRITICAL_SECTION _object;
public:
  CCriticalSection(): CSyncObject(NULL)
    { ::InitializeCriticalSection(&_object); }

  // operator CRITICAL_SECTION*() { return (CRITICAL_SECTION*) &_object; }

  bool Unlock()
  { 
    ::LeaveCriticalSection(&_object); 
    return true; 
  }
  bool Lock()
  { 
    ::EnterCriticalSection(&_object); 
    return true; 
  }
  bool Lock(DWORD timeoutInterval)
    { return Lock(); }

public:
  virtual ~CCriticalSection()
    { ::DeleteCriticalSection(&_object); }

};


class CSingleLock
{
public:
  CSingleLock(CSyncObject* object, bool initialLock = false);
public:
  bool Lock(DWORD timeoutInterval = INFINITE);
  bool Unlock();
  bool Unlock(LONG count, LPLONG prevCount = NULL);
  bool IsLocked()
    { return _acquired; }

  ~CSingleLock()
    { Unlock(); }

protected:
  CSyncObject* _syncObject;
  HANDLE _object;
  bool _acquired;
};

}}

#endif
