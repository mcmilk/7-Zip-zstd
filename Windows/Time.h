// Windows/Time.h

#pragma once

#ifndef __WINDOWS_TIME_H
#define __WINDOWS_TIME_H

// #include "Common/Types.h"
// #include <windows.h>
// #include <time.h>
#include "Windows/Defs.h"

namespace NWindows {
namespace NTime {

inline bool DosTimeToFileTime(UINT32 dosTime, FILETIME &fileTime)
{
  return BOOLToBool(::DosDateTimeToFileTime(UINT16(dosTime >> 16), 
      UINT16(dosTime & 0xFFFF), &fileTime));
}

inline bool FileTimeToDosTime(const FILETIME &fileTime, UINT32 &dosTime)
{
  return BOOLToBool(::FileTimeToDosDateTime(&fileTime, 
      ((LPWORD)&dosTime) + 1, (LPWORD)&dosTime));
}

const UINT64 kUnixTimeStartValue = 116444736000000000;
const UINT32 kNumTimeQuantumsInSecond = 10000000;

inline void UnixTimeToFileTime(long unixTime, FILETIME &fileTime)
{
  ULONGLONG ll = UInt32x32To64(unixTime, kNumTimeQuantumsInSecond) + 
      kUnixTimeStartValue;
  fileTime.dwLowDateTime = (DWORD) ll;
  fileTime.dwHighDateTime = DWORD(ll >> 32);
}

inline bool FileTimeToUnixTime(const FILETIME &fileTime, long &unixTime)
{
  UINT64 winTime = (((UINT64)fileTime.dwHighDateTime) << 32) + fileTime.dwLowDateTime;
  if (winTime < kUnixTimeStartValue)
    return false;
  winTime = (winTime - kUnixTimeStartValue) / kNumTimeQuantumsInSecond;
  if (winTime >= 0xFFFFFFFF)
    return false;
  unixTime = (long)winTime;
  return true;
}

}}

#endif
