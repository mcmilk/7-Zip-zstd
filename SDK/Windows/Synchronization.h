// Windows/Synchronization.h

#pragma once

#ifndef __WINDOWS_SYNCHRONIZATION_H
#define __WINDOWS_SYNCHRONIZATION_H

#include "Windows/Defs.h"

namespace NWindows {
namespace NSynchronization {

class CSingleLock;

class CObject
{
public:
  HANDLE  _object;
  CObject(): _object(NULL) {};
  operator HANDLE() const { return _object;}
  bool Lock(DWORD timeoutInterval = INFINITE);
  ~CObject();
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


/*
class CSemaphore: public CSyncObject
{
public:
  CSemaphore(LONG initialCount = 1, LONG maxCount = 1,
    LPCTSTR name = NULL, LPSECURITY_ATTRIBUTES securityAttributes = NULL);

public:
  virtual ~CSemaphore();
  virtual bool Unlock()
    { return Unlock(1, NULL); }
  virtual bool Unlock(LONG count, LPLONG prevCount = NULL);
};


class CMutex: public CSyncObject
{
public:
  CMutex(bool initiallyOwn = false, LPCTSTR name = NULL,
    LPSECURITY_ATTRIBUTES securityAttributes = NULL);

  virtual ~CMutex();
  bool Unlock();
};
*/

class CEvent: public CObject
{
public:
  CEvent(bool initiallyOwn = false, bool manualReset = false,
    LPCTSTR name = NULL, LPSECURITY_ATTRIBUTES securityAttributes = NULL);

  bool Set() { return BOOLToBool(::SetEvent(_object)); }
  bool Pulse() { return BOOLToBool(::PulseEvent(_object)); }
  bool Reset() { return BOOLToBool(::ResetEvent(_object)); }
  // bool Unlock();
  // virtual ~CEvent();
};

class CManualResetEvent: public CEvent
{
public:
  CManualResetEvent(bool initiallyOwn = false, LPCTSTR name = NULL, 
      LPSECURITY_ATTRIBUTES securityAttributes = NULL):
    CEvent(initiallyOwn, true, name, securityAttributes) {};
};

class CAutoResetEvent: public CEvent
{
public:
  CAutoResetEvent(bool initiallyOwn = false, LPCTSTR name = NULL, 
      LPSECURITY_ATTRIBUTES securityAttributes = NULL):
    CEvent(initiallyOwn, false, name, securityAttributes) {};
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
