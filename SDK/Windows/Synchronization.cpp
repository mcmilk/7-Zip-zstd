// Windows/Synchronization.cpp

#include "StdAfx.h"

#include "Windows/Synchronization.h"

namespace NWindows {
namespace NSynchronization {

CObject::~CObject()
{
  if (_object != NULL)
  {
    ::CloseHandle(_object);
    _object = NULL;
  }
}

bool CObject::Lock(DWORD timeoutInterval)
{
  return (::WaitForSingleObject(_object, timeoutInterval) == WAIT_OBJECT_0);
}


CSyncObject::~CSyncObject()
{
  if (_object != NULL)
  {
    ::CloseHandle(_object);
    _object = NULL;
  }
}

bool CSyncObject::Lock(DWORD timeoutInterval)
{
  if (::WaitForSingleObject(_object, timeoutInterval) == WAIT_OBJECT_0)
    return true;
  else
    return false;
}

/*
/////////////////////
// CSemaphore

CSemaphore::CSemaphore(LONG initialCount, LONG maxCount,
  LPCTSTR name, LPSECURITY_ATTRIBUTES securityAttributes)
  :  CSyncObject(name)
{
  // // ASSERT(maxCount > 0);
  // // ASSERT(initialCount <= maxCount);

  _object = ::CreateSemaphore(securityAttributes, initialCount, maxCount,
      name);
  if (_object == NULL)
    throw "CreateSemaphore error";
}

CSemaphore::~CSemaphore()
{}

bool CSemaphore::Unlock(LONG count, LPLONG prevCount)
{
  return BOOLToBool(::ReleaseSemaphore(_object, count, prevCount));
}

/////////////////////
// CMutex

CMutex::CMutex(bool initiallyOwn, LPCTSTR name,
  LPSECURITY_ATTRIBUTES securityAttributes)
  : CSyncObject(name)
{
  _object = ::CreateMutex(securityAttributes, BoolToBOOL(initiallyOwn), name);
  if (_object == NULL)
    throw "CreateMutex error";
}

CMutex::~CMutex()
{}

bool CMutex::Unlock()
{
  return BOOLToBool(::ReleaseMutex(_object));
}
*/

/////////////////////
// CEvent

CEvent::CEvent(bool initiallyOwn, bool manualReset, LPCTSTR name,
  LPSECURITY_ATTRIBUTES securityAttributes)
  /*: CSyncObject(name)*/
{
  _object = ::CreateEvent(securityAttributes, BoolToBOOL(manualReset),
      BoolToBOOL(initiallyOwn), name);
  if (_object == NULL)
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

CSingleLock::CSingleLock(CSyncObject* object, bool initialLock)
{
  // ASSERT(object != NULL);
  // ASSERT(object->IsKindOf(RUNTIME_CLASS(CSyncObject)));

  _syncObject = object;
  _object = object->_object;
  _acquired = false;

  if (initialLock)
    Lock();
}

bool CSingleLock::Lock(DWORD timeoutInterval /* = INFINITE */)
{
  // ASSERT(_syncObject != NULL || _object != NULL);
  // ASSERT(!_acquired);

  _acquired = _syncObject->Lock(timeoutInterval);
  return _acquired;
}

bool CSingleLock::Unlock()
{
  // ASSERT(_syncObject != NULL);
  if (_acquired)
    _acquired = !_syncObject->Unlock();

  // successfully unlocking means it isn't acquired
  return !_acquired;
}

bool CSingleLock::Unlock(LONG count, LPLONG prevCount /* = NULL */)
{
  // ASSERT(_syncObject != NULL);
  if (_acquired)
    _acquired = !_syncObject->Unlock(count, prevCount);

  // successfully unlocking means it isn't acquired
  return !_acquired;
}

}}