// Windows/DLL.cpp

#include "StdAfx.h"

#include "DLL.h"
#include "Defs.h"
#ifndef _UNICODE
#include "../Common/StringConvert.h"
#endif

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
  // OutputDebugString(fileName);
  // OutputDebugString(TEXT("\n"));
  return LoadOperations(::LoadLibrary(fileName));
}

#ifndef _UNICODE
static inline UINT GetCurrentCodePage() 
  { return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP; } 
bool CLibrary::LoadEx(LPCWSTR fileName, DWORD flags)
{
  HMODULE module = ::LoadLibraryExW(fileName, NULL, flags);
  if (module != 0)
    return LoadOperations(module);
  if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return false;
  return LoadEx(UnicodeStringToMultiByte(fileName, GetCurrentCodePage()), flags);
}
bool CLibrary::Load(LPCWSTR fileName)
{
  HMODULE module = ::LoadLibraryW(fileName);
  if (module != 0)
    return LoadOperations(module);
  if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return false;
  return Load(UnicodeStringToMultiByte(fileName, GetCurrentCodePage()));
}
#endif

bool MyGetModuleFileName(HMODULE hModule, CSysString &result)
{
  result.Empty();
  TCHAR fullPath[MAX_PATH + 2];
  DWORD size = ::GetModuleFileName(hModule, fullPath, MAX_PATH + 1);
  if (size <= MAX_PATH && size != 0)
  {
    result = fullPath;
    return true;
  }
  return false;
}

#ifndef _UNICODE
bool MyGetModuleFileName(HMODULE hModule, UString &result)
{
  result.Empty();
  wchar_t fullPath[MAX_PATH + 2];
  DWORD size = ::GetModuleFileNameW(hModule, fullPath, MAX_PATH + 1);
  if (size <= MAX_PATH && size != 0)
  {
    result = fullPath;
    return true;
  }
  if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return false;
  CSysString resultSys;
  if (!MyGetModuleFileName(hModule, resultSys))
    return false;
  result = MultiByteToUnicodeString(resultSys, GetCurrentCodePage());
  return true;
}
#endif

}}