// Windows/DLL.cpp

#include "StdAfx.h"

#include "DLL.h"
#include "Defs.h"

namespace NWindows {
namespace NDLL {

CLibrary::~CLibrary()
{
  Free();
}

bool CLibrary::Free()
{
  if (_module == 0)
    return true;
  // MessageBox(0, TEXT(""), TEXT("Free"), 0);
  // Sleep(5000);
  if (!::FreeLibrary(_module))
    return false;
  _module = 0;
  return true;
}

bool CLibrary::LoadOperations(HMODULE newModule)
{
  if (newModule == NULL)
    return false;
  if(!Free())
    return false;
  _module = newModule;
  return true;
}

bool CLibrary::LoadEx(LPCTSTR fileName, DWORD flags)
{
  // MessageBox(0, fileName, TEXT("LoadEx"), 0);
  return LoadOperations(::LoadLibraryEx(fileName, NULL, flags));
}

bool CLibrary::Load(LPCTSTR fileName)
{
  // MessageBox(0, fileName, TEXT("Load"), 0);
  // Sleep(5000);
  OutputDebugString(fileName);
  OutputDebugString(TEXT("\n"));
  return LoadOperations(::LoadLibrary(fileName));
}

}}