// Windows/DLL.cpp

#include "StdAfx.h"

#include "Windows/DLL.h"

namespace NWindows{
namespace NDLL{

CLibrary::~CLibrary()
{
  FreeAll();
}

bool CLibrary::FreeAll()
{
  while(_refCount > 0)
    if(!Free())
      return false;
  return true;
}

bool CLibrary::Free()
{
  if(_refCount == 0)
    return true;
  if (::FreeLibrary(_module) == FALSE)
    return false;
  _refCount--;
  return true;
}

bool CLibrary::LoadOperations(HMODULE newModule)
{
  if (newModule == NULL)
    return false;
  if(_refCount > 0 && newModule != _module)
  {
    FreeAll();
    _refCount = 0;
  }
  _module = newModule;
  _refCount++;
  return true;
}

bool CLibrary::LoadEx(LPCTSTR fileName, DWORD flags)
{
  return LoadOperations(::LoadLibraryEx(fileName, NULL, flags));
}

bool CLibrary::Load(LPCTSTR fileName)
{
  return LoadOperations(::LoadLibrary(fileName));
}

}}