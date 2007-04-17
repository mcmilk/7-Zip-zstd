// Windows/System.h

#ifndef __WINDOWS_SYSTEM_H
#define __WINDOWS_SYSTEM_H

#include "..\Common\Types.h"

namespace NWindows {
namespace NSystem {

inline UInt32 GetNumberOfProcessors()
{
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  return (UInt32)systemInfo.dwNumberOfProcessors;
}

#ifndef _WIN64
typedef BOOL (WINAPI *GlobalMemoryStatusExP)(LPMEMORYSTATUSEX lpBuffer);
#endif

inline UInt64 GetRamSize()
{
  MEMORYSTATUSEX stat;
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

#endif
