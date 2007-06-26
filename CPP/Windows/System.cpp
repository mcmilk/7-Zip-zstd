// Windows/System.cpp

#include "StdAfx.h"

#include "System.h"

namespace NWindows {
namespace NSystem {

UInt32 GetNumberOfProcessors()
{
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  return (UInt32)systemInfo.dwNumberOfProcessors;
}

#if !defined(_WIN64) && defined(__GNUC__)

typedef struct _MY_MEMORYSTATUSEX {
  DWORD dwLength;
  DWORD dwMemoryLoad;
  DWORDLONG ullTotalPhys;
  DWORDLONG ullAvailPhys;
  DWORDLONG ullTotalPageFile;
  DWORDLONG ullAvailPageFile;
  DWORDLONG ullTotalVirtual;
  DWORDLONG ullAvailVirtual;
  DWORDLONG ullAvailExtendedVirtual;
} MY_MEMORYSTATUSEX, *MY_LPMEMORYSTATUSEX;

#else

#define MY_MEMORYSTATUSEX MEMORYSTATUSEX
#define MY_LPMEMORYSTATUSEX LPMEMORYSTATUSEX

#endif

typedef BOOL (WINAPI *GlobalMemoryStatusExP)(MY_LPMEMORYSTATUSEX lpBuffer);

UInt64 GetRamSize()
{
  MY_MEMORYSTATUSEX stat;
  stat.dwLength = sizeof(stat);
  #ifdef _WIN64
  if (!::GlobalMemoryStatusEx(&stat))
    return 0;
  return stat.ullTotalPhys;
  #else
  GlobalMemoryStatusExP globalMemoryStatusEx = (GlobalMemoryStatusExP)
        ::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")),
        "GlobalMemoryStatusEx");
  if (globalMemoryStatusEx != 0)
    if (globalMemoryStatusEx(&stat))
      return stat.ullTotalPhys;
  {
    MEMORYSTATUS stat;
    stat.dwLength = sizeof(stat);
    GlobalMemoryStatus(&stat);
    return stat.dwTotalPhys;
  }
  #endif
}

}}
