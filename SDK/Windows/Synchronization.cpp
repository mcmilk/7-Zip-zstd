// Windows/Synchronization.cpp

#include "StdAfx.h"

#include "Windows/Synchronization.h"

namespace NWindows {
namespace NSynchronization {


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
  return (::WaitForSingleObject(_object, timeoutInterval) == WAIT_OBJECT_0);
}

/////////////////////
// CEvent

CEvent::CEvent(bool manualReset, bool initiallyOwn, LPCTSTR name,
    LPSECURITY_ATTRIBUTES securityAttributes)
{
  if (!Create(manualReset, initiallyOwn, name, securityAttributes))
    throw "CreateEvent error";
}

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