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
  HANDLE  m_Object;
  CObject(): m_Object(NULL) {};
  operator HANDLE() const { return m_Object;}
  bool Lock(DWORD aTimeout = INFINITE);
  ~CObject();
};

class CSyncObject
{
public:
  HANDLE  m_Object;

  CSyncObject(LPCTSTR aName): m_Object(NULL) {};
  operator HANDLE() const
    { return m_Object;}

  virtual bool Lock(DWORD aTimeout = INFINITE);
  virtual bool Unlock() = 0;
  virtual bool Unlock(LONG /* aCount */, LPLONG /* aPrevCount = NULL */)
    { return true; }

public:
  virtual ~CSyncObject();
  friend class CSingleLock;
};


/*
class CSemaphore: public CSyncObject
{
public:
  CSemaphore(LONG anInitialCount = 1, LONG aMaxCount = 1,
    LPCTSTR aName = NULL, LPSECURITY_ATTRIBUTES lpsaAttributes = NULL);

public:
  virtual ~CSemaphore();
  virtual bool Unlock()
    { return Unlock(1, NULL); }
  virtual bool Unlock(LONG aCount, LPLONG aPrevCount = NULL);
};


class CMutex: public CSyncObject
{
public:
  CMutex(bool anInitiallyOwn = false, LPCTSTR aName = NULL,
    LPSECURITY_ATTRIBUTES aSecurityAttributes = NULL);

  virtual ~CMutex();
  bool Unlock();
};
*/

class CEvent: public CObject
{
public:
  CEvent(bool anInitiallyOwn = false, bool aManualReset = false,
    LPCTSTR aName = NULL, LPSECURITY_ATTRIBUTES aSecurityAttributes = NULL);

  bool Set() { return BOOLToBool(::SetEvent(m_Object)); }
  bool Pulse() { return BOOLToBool(::PulseEvent(m_Object)); }
  bool Reset() { return BOOLToBool(::ResetEvent(m_Object)); }
  // bool Unlock();
  // virtual ~CEvent();
};

class CManualResetEvent: public CEvent
{
public:
  CManualResetEvent(bool anInitiallyOwn = false, LPCTSTR aName = NULL, 
      LPSECURITY_ATTRIBUTES aSecurityAttributes = NULL):
    CEvent(anInitiallyOwn, true, aName, aSecurityAttributes) {};
};

class CAutoResetEvent: public CEvent
{
public:
  CAutoResetEvent(bool anInitiallyOwn = false, LPCTSTR aName = NULL, 
      LPSECURITY_ATTRIBUTES aSecurityAttributes = NULL):
    CEvent(anInitiallyOwn, false, aName, aSecurityAttributes) {};
};


class CCriticalSection: public CSyncObject
{
  CRITICAL_SECTION m_Object;
public:
  CCriticalSection(): CSyncObject(NULL)
    { ::InitializeCriticalSection(&m_Object); }

  // operator CRITICAL_SECTION*() { return (CRITICAL_SECTION*) &m_Object; }

  bool Unlock()
  { 
    ::LeaveCriticalSection(&m_Object); 
    return true; 
  }
  bool Lock()
  { 
    ::EnterCriticalSection(&m_Object); 
    return true; 
  }
  bool Lock(DWORD aTimeout)
    { return Lock(); }

public:
  virtual ~CCriticalSection()
    { ::DeleteCriticalSection(&m_Object); }

};


class CSingleLock
{
public:
  CSingleLock(CSyncObject* anObject, bool anInitialLock = false);
public:
  bool Lock(DWORD aTimeOut = INFINITE);
  bool Unlock();
  bool Unlock(LONG aCount, LPLONG aPrevCount = NULL);
  bool IsLocked()
    { return m_Acquired; }

  ~CSingleLock()
    { Unlock(); }

protected:
  CSyncObject* m_SyncObject;
  HANDLE  m_Object;
  bool    m_Acquired;
};

}}

#endif
