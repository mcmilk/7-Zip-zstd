// Windows/Synchronization.cpp

#include "StdAfx.h"

#include "Windows/Synchronization.h"

namespace NWindows {
namespace NSynchronization {

CObject::~CObject()
{
  if (m_Object != NULL)
  {
    ::CloseHandle(m_Object);
    m_Object = NULL;
  }
}

bool CObject::Lock(DWORD aTimeout)
{
  return (::WaitForSingleObject(m_Object, aTimeout) == WAIT_OBJECT_0);
}


CSyncObject::~CSyncObject()
{
  if (m_Object != NULL)
  {
    ::CloseHandle(m_Object);
    m_Object = NULL;
  }
}

bool CSyncObject::Lock(DWORD aTimeout)
{
  if (::WaitForSingleObject(m_Object, aTimeout) == WAIT_OBJECT_0)
    return true;
  else
    return false;
}

/*
/////////////////////
// CSemaphore

CSemaphore::CSemaphore(LONG anInitialCount, LONG aMaxCount,
  LPCTSTR aName, LPSECURITY_ATTRIBUTES lpsaAttributes)
  :  CSyncObject(aName)
{
  // // ASSERT(aMaxCount > 0);
  // // ASSERT(anInitialCount <= aMaxCount);

  m_Object = ::CreateSemaphore(lpsaAttributes, anInitialCount, aMaxCount,
      aName);
  if (m_Object == NULL)
    throw "CreateSemaphore error";
}

CSemaphore::~CSemaphore()
{}

bool CSemaphore::Unlock(LONG aCount, LPLONG aPrevCount)
{
  return BOOLToBool(::ReleaseSemaphore(m_Object, aCount, aPrevCount));
}

/////////////////////
// CMutex

CMutex::CMutex(bool anInitiallyOwn, LPCTSTR aName,
  LPSECURITY_ATTRIBUTES aSecurityAttributes)
  : CSyncObject(aName)
{
  m_Object = ::CreateMutex(aSecurityAttributes, BoolToBOOL(anInitiallyOwn), aName);
  if (m_Object == NULL)
    throw "CreateMutex error";
}

CMutex::~CMutex()
{}

bool CMutex::Unlock()
{
  return BOOLToBool(::ReleaseMutex(m_Object));
}
*/

/////////////////////
// CEvent

CEvent::CEvent(bool anInitiallyOwn, bool aManualReset, LPCTSTR aName,
  LPSECURITY_ATTRIBUTES aSecurityAttributes)
  /*: CSyncObject(aName)*/
{
  m_Object = ::CreateEvent(aSecurityAttributes, BoolToBOOL(aManualReset),
      BoolToBOOL(anInitiallyOwn), aName);
  if (m_Object == NULL)
    throw "CreateEvent error";
}

/*
CEvent::~CEvent()
{}

bool CEvent::Unlock()
{
  return true;
}
*/

/////////////////////
// CSingleLock

CSingleLock::CSingleLock(CSyncObject* anObject, bool anInitialLock)
{
  // ASSERT(anObject != NULL);
  // ASSERT(anObject->IsKindOf(RUNTIME_CLASS(CSyncObject)));

  m_SyncObject = anObject;
  m_Object = anObject->m_Object;
  m_Acquired = false;

  if (anInitialLock)
    Lock();
}

bool CSingleLock::Lock(DWORD aTimeOut /* = INFINITE */)
{
  // ASSERT(m_SyncObject != NULL || m_Object != NULL);
  // ASSERT(!m_Acquired);

  m_Acquired = m_SyncObject->Lock(aTimeOut);
  return m_Acquired;
}

bool CSingleLock::Unlock()
{
  // ASSERT(m_SyncObject != NULL);
  if (m_Acquired)
    m_Acquired = !m_SyncObject->Unlock();

  // successfully unlocking means it isn't acquired
  return !m_Acquired;
}

bool CSingleLock::Unlock(LONG aCount, LPLONG aPrevCount /* = NULL */)
{
  // ASSERT(m_SyncObject != NULL);
  if (m_Acquired)
    m_Acquired = !m_SyncObject->Unlock(aCount, aPrevCount);

  // successfully unlocking means it isn't acquired
  return !m_Acquired;
}

}}