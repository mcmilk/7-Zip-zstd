// Windows/Time.h

#pragma once

#ifndef __WINDOWS_TIME_H
#define __WINDOWS_TIME_H

#include "Common/Types.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NTime {

inline bool DosTimeToFileTime(UINT32 aDosTime, FILETIME &aFileTime)
{
  return BOOLToBool(::DosDateTimeToFileTime(UINT16(aDosTime >> 16), 
      UINT16(aDosTime & 0xFFFF), &aFileTime));
}

inline bool FileTimeToDosTime(const FILETIME &aFileTime, UINT32 &aDosTime)
{
  return BOOLToBool(::FileTimeToDosDateTime(&aFileTime, 
      ((LPWORD)&aDosTime) + 1, (LPWORD)&aDosTime));
}

const UINT64 kUnixTimeStartValue = 116444736000000000;
const kNumTimeQuantumsInSecond = 10000000;

inline void UnixTimeToFileTime(time_t anUnixTime, FILETIME &aFileTime)
{
  ULONGLONG ll = UInt32x32To64(anUnixTime, kNumTimeQuantumsInSecond) + 
      kUnixTimeStartValue;
  aFileTime.dwLowDateTime = (DWORD) ll;
  aFileTime.dwHighDateTime = DWORD(ll >> 32);
}

inline bool FileTimeToUnixTime(const FILETIME &aFileTime, time_t &anUnixTime)
{
  UINT64 aWinTime = (((UINT64)aFileTime.dwHighDateTime) << 32) + aFileTime.dwLowDateTime;
  if (aWinTime < kUnixTimeStartValue)
    return false;
  aWinTime = (aWinTime - kUnixTimeStartValue) / kNumTimeQuantumsInSecond;
  if (aWinTime >= 0xFFFFFFFF)
    return false;
  anUnixTime = (time_t)aWinTime;
  return true;
}

}}

#endif
